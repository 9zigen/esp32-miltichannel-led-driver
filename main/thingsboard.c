/***
** Created by Aleksey Volkov on 01.01.2020.
***/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <light.h>
#include <monitor.h>
#include <cJSON.h>
#include <rtc.h>
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"

#include "main.h"
#include "settings.h"
#include "thingsboard.h"

#define TELEMETRY_TOPIC     "v1/devices/me/telemetry"        /* ThingsBoard MQTT telemetry topic */
#define RPC_REQUEST_TOPIC   "v1/devices/me/rpc/request/+"    /* ThingsBoard RPC Request topic */

#ifdef THINGSBOARD
static const char *TAG = "THINGSBOARD";

thingsboard_t * thingsboard_config;
esp_mqtt_client_handle_t thingsboard_client;
uint8_t thingsboard_mqtt_connected = 0;
uint8_t thingsboard_mqtt_started = 0;
uint32_t thingsboard_disconnect_count = 0;
uint32_t thingsboard_unsubscrib_count = 0;

TimerHandle_t timer_publish_telemetry;

char * rpc_topic_p = RPC_REQUEST_TOPIC;

/* Private */
static void thingsboard_mqtt_subscribe();
static void on_thingsboard_mqtt_message(esp_mqtt_event_handle_t incoming_event);

static void vTimerTelemetryCallback(TimerHandle_t xTimer )
{
  thingsboard_publish_device_status();
}

/* ThingsBoard MQTT Event Handler */
static esp_err_t thingsboard_mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  switch (event->event_id) {
    case MQTT_EVENT_BEFORE_CONNECT:
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      thingsboard_mqtt_connected = 1;
      /* subscribe to RPC */
      thingsboard_mqtt_subscribe();
      /* start device info timer */
      xTimerStart( timer_publish_telemetry, 100 );
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      thingsboard_mqtt_connected = 0;
      thingsboard_disconnect_count++;
      xTimerStop(timer_publish_telemetry, 100 );
      break;
    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
      thingsboard_unsubscrib_count++;
      thingsboard_mqtt_subscribe();
      break;
    case MQTT_EVENT_PUBLISHED:
      ESP_LOGI(TAG, "CLOUD_MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "MQTT_EVENT_DATA");
      on_thingsboard_mqtt_message(event);
      break;
    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
      break;
    case MQTT_EVENT_DELETED:
      ESP_LOGI(TAG, "MQTT_EVENT_DELETED");
      break;
    case MQTT_EVENT_ANY:
      break;
  }
  return ESP_OK;
}

void init_thingsboard()
{
  thingsboard_config = get_thingsboard();

  ESP_LOGI(TAG, "enabled: %d | endpoint: %s | token:%s",
      thingsboard_config->enable, thingsboard_config->endpoint, thingsboard_config->token);

  if (thingsboard_config->enable && strlen(thingsboard_config->endpoint) > 10)
  {
    esp_mqtt_client_config_t mqtt_thingsboard_cfg = {
        .uri = thingsboard_config->endpoint,                      /* thingboard mqtt server ex: mqtt://test.com:1883 */
        .username = thingsboard_config->token,                    /* thingboard device token */
//        .cert_pem = (const char *) mqtt_server_cert_pem_start,  /* ssl certificate */
        .event_handle = thingsboard_mqtt_event_handler
    };

    /* Create device status publish timer, 1 min period */
    timer_publish_telemetry = xTimerCreate("TimerDeviceTelemetry", 60 * 1000 / portTICK_RATE_MS, pdTRUE, NULL,
                                           vTimerTelemetryCallback);

    ESP_LOGI(TAG, "[APP] free memory before start ThingBoard MQTT client: %d bytes", esp_get_free_heap_size());
    thingsboard_client = esp_mqtt_client_init(&mqtt_thingsboard_cfg);
    if (thingsboard_client != NULL)
    {
      esp_mqtt_client_start(thingsboard_client);
    }
  }
}

void task_thingsboard(void *pvParameters)
{
  init_thingsboard();
  for (;;)
  {
    if (thingsboard_mqtt_connected)
    {
      thingsboard_mqtt_started = 1;

      /* publish current info */
      thingsboard_publish_device_status();
    }
    if (thingsboard_mqtt_started || !thingsboard_config->enable)
    {
      vTaskDelete(NULL);
    }
    vTaskDelay(200/portTICK_RATE_MS);
  }
}

/* On RPC Message */
void on_thingsboard_mqtt_message(esp_mqtt_event_handle_t incoming_event) {
  ESP_LOGD(TAG, "publish received:");
  ESP_LOGD(TAG,"TOPIC=%.*s", incoming_event->topic_len, incoming_event->topic);
  ESP_LOGD(TAG,"DATA=%.*s", incoming_event->data_len, incoming_event->data);

  int msg_id;
  char message_buf[128];
  char topic_buf[128];
  char data_buf[512];
  char command_buf[12];
  uint32_t message_id;
  char response_topic_buf[128];

  memcpy(topic_buf, incoming_event->topic, incoming_event->topic_len);
  memcpy(data_buf, incoming_event->data, incoming_event->data_len);
  topic_buf[incoming_event->topic_len] = 0;
  data_buf[incoming_event->data_len] = 0;

  /* parse message id */
  /* "sscanf" template example: v1/devices/me/rpc/request/%u */
  sscanf(topic_buf, "v1/devices/me/rpc/request/%u", &message_id);

  /* Parse RPC Json Request */
  cJSON *request_root = cJSON_Parse(data_buf);
  cJSON *method = cJSON_GetObjectItem(request_root, "method");

  if (cJSON_IsString(method) && (method->valuestring != NULL)) {
    /* Brightness */
    if (strncmp(method->valuestring, "getBrightnessValue", 18) == 0)
    {
      snprintf(message_buf, 128, "%u", get_brightness());
    }
    else if (strncmp(method->valuestring, "setBrightnessValue", 18) == 0)
    {
      cJSON *params = cJSON_GetObjectItem(request_root, "params");
      uint8_t brightness = atoi(params->valuestring);
      set_brightness(brightness);
    }
    else if (strncmp(method->valuestring, "getChannel", 10) == 0) {
      /* Channels, get value */
      for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
        snprintf(command_buf, 12, "getChannel%u", i);
        if (strncmp(method->valuestring, command_buf, 11) == 0) {
          snprintf(message_buf, 128, "%u", get_channel_duty(i));
        }
      }
    }
    else if (strncmp(method->valuestring, "setChannel", 10) == 0)
    {
      /* Channels, set value */
      for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
        snprintf(command_buf, 12, "setChannel%u", i);
        if (strncmp(method->valuestring, command_buf, 11) == 0) {
          cJSON *params = cJSON_GetObjectItem(request_root, "params");
          uint8_t duty = atoi(params->valuestring);
          set_channel_duty(i, duty);
        }
      }
    }
  }

  cJSON_Delete(request_root);

  /* send response */
  snprintf(response_topic_buf, 128, "v1/devices/me/rpc/response/%u", message_id);
  msg_id = esp_mqtt_client_publish(thingsboard_client, response_topic_buf, message_buf, strlen(message_buf), thingsboard_config->qos, thingsboard_config->retain);

  ESP_LOGD(TAG, "publish to: %s", response_topic_buf);
  ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);

  /* make response message json string */
//  cJSON *root = cJSON_CreateObject();
//  message_buf = cJSON_Print(root);
//  if (message_buf == NULL)
//  {
//    ESP_LOGE(TAG, "failed to print status json.");
//  }
//  cJSON_Delete(root);

//  /* send response */
//  snprintf(response_topic_buf, 128, "v1/devices/me/rpc/response/%u", message_id);
//  msg_id = esp_mqtt_client_publish(thingsboard_client, response_topic_buf, message_buf, strlen(message_buf), thingsboard_config->qos, thingsboard_config->retain);
////  free(message_buf);
//
//  ESP_LOGD(TAG, "publish to: %s", response_topic_buf);
//  ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);

}

/* Subscribe to RPC */
static void thingsboard_mqtt_subscribe()
{
  int msg_id;

  if (thingsboard_config->rpc)
  {
    /* subscribe to RPC topic */
    msg_id = esp_mqtt_client_subscribe(thingsboard_client, rpc_topic_p, thingsboard_config->qos);
    ESP_LOGI(TAG, "subscribe to rpc topic: %s", rpc_topic_p);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
  } else {
    ESP_LOGI(TAG, "subscribe to rpc topic: %s disabled", rpc_topic_p);
  }

}

/* Publish telemetry */
void thingsboard_publish_device_status() {
  if (!thingsboard_mqtt_connected)
    return;

  int msg_id;
  char * message_buf;
  char time_string[32];
  char channel_name[10];

  system_status_t *system_status = get_system_status();
  get_time_string((char*) &time_string);

  /* make message json string */
  cJSON *root = cJSON_CreateObject();

  det_time_string_since_boot((char*) &time_string);
  cJSON_AddItemToObject(root, "up_time", cJSON_CreateString(time_string));

  get_time_string((char*) &time_string);
  cJSON_AddItemToObject(root, "local_time", cJSON_CreateString(time_string));
  cJSON_AddItemToObject(root, "firmware", cJSON_CreateString(FIRMWARE));
  cJSON_AddItemToObject(root, "free_heap", cJSON_CreateNumber(system_status->free_heap));
//  cJSON_AddItemToObject(root, "vcc", cJSON_CreateNumber(get_stm_vcc_power()));
//  cJSON_AddItemToObject(root, "ntc_temperature", cJSON_CreateNumber(get_stm_ntc_temperature()));
//  cJSON_AddItemToObject(root, "board_temperature", cJSON_CreateNumber(get_stm_mcu_temperature()));
  cJSON_AddItemToObject(root, "local_ip_address", cJSON_CreateString(system_status->net_address));
  cJSON_AddItemToObject(root, "brightness", cJSON_CreateNumber(get_brightness()));
  cJSON_AddItemToObject(root, "disconnects_cnt", cJSON_CreateNumber(thingsboard_disconnect_count));
  cJSON_AddItemToObject(root, "unsubscribe_cnt", cJSON_CreateNumber(thingsboard_unsubscrib_count));

  /* Channels duty */
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i) {
    snprintf(channel_name, 10, "channel_%u", i);
    cJSON_AddItemToObject(root, channel_name, cJSON_CreateNumber(get_channel_duty(i)));
  }

  message_buf = cJSON_Print(root);
  if (message_buf == NULL)
  {
    ESP_LOGE(TAG, "failed to print status json.");
  }

  cJSON_Delete(root);

  /* publish led status to topic */
  msg_id = esp_mqtt_client_publish(thingsboard_client, TELEMETRY_TOPIC, message_buf, strlen(message_buf), thingsboard_config->qos, thingsboard_config->retain);
  free(message_buf);
  ESP_LOGD(TAG, "publish to: %s", TELEMETRY_TOPIC);
  ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);
}

#endif