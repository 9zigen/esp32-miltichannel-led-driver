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
#include "mqtt.h"

#define TELEMETRY_TOPIC  "v1/devices/me/telemetry"    /* ThingsBoard MQTT telemetry topic */
static const char *TAG = "MQTT";

esp_mqtt_client_handle_t client;
uint8_t mqtt_enabled = 0;
uint8_t mqtt_connected = 0;
static uint8_t mqtt_started = 0;
TimerHandle_t timer_publish_status;

/* Private */
static void mqtt_subscribe();
void on_mqtt_message(esp_mqtt_event_handle_t incoming_event);

/* MQTT Event Handler */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
  // esp_mqtt_client_handle_t ev_client = event->client;
  // int msg_id;
  // your_context_t *context = event->context;
  switch (event->event_id) {
    case MQTT_EVENT_BEFORE_CONNECT:
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      mqtt_connected = 1;
      /* start device info timer */
      xTimerStart(timer_publish_status, 100 );
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      mqtt_connected = 0;
      xTimerStop(timer_publish_status, 100 );
      break;
    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_PUBLISHED:
      ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "MQTT_EVENT_DATA");
      on_mqtt_message(event);
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

static void vTimerCallback( TimerHandle_t xTimer )
{
  mqtt_publish_device_status();
}

/* Setup MQTT Client
 * .uri = "mqtts://api.emitter.io:443" for mqtt over ssl
 * .uri = "mqtt://api.emitter.io:8080" for mqtt over tcp
 * .uri = "ws://api.emitter.io:8080"   for mqtt over websocket
 * .uri = "wss://api.emitter.io:443"   for mqtt over websocket secure
 * */
void init_mqtt(void)
{
  services_t * services = get_services();

  char mqtt_address_buff[64];
  char ip_address[16];
  ip_to_string(services->mqtt_ip_address, ip_address);
  snprintf(mqtt_address_buff, 64, "mqtt://%s:%d", ip_address, services->mqtt_port);

  ESP_LOGI(TAG, "enabled: %d | address: %s | user:%s | password:%s", services->enable_mqtt, mqtt_address_buff,
           services->mqtt_user, services->mqtt_password);

  if (services->enable_mqtt && strlen(mqtt_address_buff) > 20)
  {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_address_buff,
        .event_handle = mqtt_event_handler
    };

    if (strlen(services->mqtt_user)) {
      mqtt_cfg.username = "pi";
    }

    if (strlen(services->mqtt_password)) {
      mqtt_cfg.password = "mqttpass";
    }

    mqtt_enabled = 1;

    /* Create device status publish timer, 1 min period */
    timer_publish_status = xTimerCreate("TimerDeviceStatus", 10 * 60 * 1000 / portTICK_RATE_MS, pdTRUE, NULL, vTimerCallback);

    ESP_LOGI(TAG, "[APP] free memory before start MQTT client: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client != NULL)
    {
      esp_mqtt_client_start(client);
    }
  }
}

void task_mqtt(void *pvParameters)
{
  init_mqtt();

  for (;;)
  {
    if (mqtt_connected && mqtt_enabled && !mqtt_started)
    {
      mqtt_started = 1;

      /* subscribe to duty and brightness topics */
      mqtt_subscribe();

      /* publish current info */
      mqtt_publish_device_status();
      mqtt_publish_channel_duty();
      mqtt_publish_brightness();
      mqtt_publish_channel_state();
    }

    if ((mqtt_started || !mqtt_enabled))
    {
      vTaskDelete(NULL);
    }

    vTaskDelay(200/portTICK_RATE_MS);
  }

}

/* On Message */
/* Topic examples:
 * Brightness: LED_11324571/brightness        | payload: 0-255
 * Set Duty:   LED_11324571/channel/0/set     | payload: 0-255
 * Switch:     LED_11324571/channel/0/switch  | payload: 0-1
 * */
void on_mqtt_message(esp_mqtt_event_handle_t incoming_event) {
  ESP_LOGI(TAG, "publish received:");
  ESP_LOGI(TAG,"TOPIC=%.*s", incoming_event->topic_len, incoming_event->topic);
  ESP_LOGI(TAG,"DATA=%.*s", incoming_event->data_len, incoming_event->data);

  char string_buf[128];
  char topic_buf[128];
  char data_buf[128];
  char command[7];
  uint32_t channel = 0;

  snprintf(topic_buf, 128, "%.*s", incoming_event->topic_len, incoming_event->topic);
  snprintf(data_buf, 128, "%.*s", incoming_event->data_len, incoming_event->data);

  services_t * settings = get_services();
  
  /* check brightness topic */
  snprintf(string_buf, 128, "%s/brightness/set", settings->hostname);

  /* Brightness received */
  if (strncmp(topic_buf, string_buf, incoming_event->topic_len) == 0)
  {
    uint8_t brightness = atoi(data_buf);

    ESP_LOGI(TAG,"set brightness: %d\n", brightness);
    set_brightness(brightness, 0);

    /* update topics */
    mqtt_publish_brightness();
    return;
  }

  /* check switch or set command */
  /* "sscanf" template example: LED_11324571/channel/%u/%6s */
  snprintf(string_buf, 128, "%s/channel/%%u/%%6s", settings->hostname);
  sscanf(topic_buf, string_buf, &channel, command);

  ESP_LOGI(TAG,"command: %s payload: %s\n", command, data_buf);

  if (strncmp(command, "set", 3) == 0)
  {
    /* Set command */
    uint8_t duty = atoi(data_buf);
    set_channel_duty(channel, duty, 0);
    ESP_LOGI(TAG,"set duty: %u channel: %u\n", duty, channel);

    /* update topics */
    mqtt_publish_channel_duty();
  }
  else if (strncmp(command, "switch", 6) == 0)
  {
    /* switch command */
    uint8_t state = atoi(data_buf);
    set_channel_state(channel, state);
    ESP_LOGI(TAG,"set state channel: %u to %u\n", channel, state);

    /* update topics */
    mqtt_publish_channel_state();
  }

}

/* Subscribe */
static void mqtt_subscribe()
{
  int msg_id;
  char topic[128];

  services_t * settings = get_services();

  /* Subscribe to Set Duty topic
   * [hostname]/channel/[channel_number]/set
   * */
  for (int i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* make topic string */
    snprintf(topic, 128, "%s/channel/%d/set", settings->hostname, i);

    /* subscribe to topic QoS */
    msg_id = esp_mqtt_client_subscribe(client, topic, settings->mqtt_qos);
    ESP_LOGI(TAG, "subscribe to topic: %s", topic);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
  }

  /* Subscribe to Switch topic
   * [hostname]/channel/[channel_number]/switch
   * */
  for (int i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* make topic string */
    snprintf(topic, 128, "%s/channel/%d/switch", settings->hostname, i);

    /* subscribe to topic QoS */
    msg_id = esp_mqtt_client_subscribe(client, topic, settings->mqtt_qos);
    ESP_LOGI(TAG, "subscribe to topic: %s", topic);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
  }

  /* Subscribe to Brightness topic
   * [hostname]/brightness/set
   * */

  /* make topic string */
  snprintf(topic, 128, "%s/brightness/set", settings->hostname);

  /* subscribe to topic QoS */
  msg_id = esp_mqtt_client_subscribe(client, topic, settings->mqtt_qos);
  ESP_LOGI(TAG, "subscribe to topic: %s", topic);
  ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
}

/* Publish all channels brightness */
void mqtt_publish_brightness()
{
  if (!mqtt_enabled || !mqtt_connected)
    return;

  int msg_id;
  char topic[128];
  char message_buf[10];

  services_t * settings = get_services();

  /* make topic string */
  snprintf(topic, 128, "%s/brightness", settings->hostname);

  /* make message string */
  snprintf(message_buf, 128, "%d", get_brightness());

  /* publish led status to topic */
  msg_id = esp_mqtt_client_publish(client, topic, message_buf, strlen(message_buf), settings->mqtt_qos, settings->mqtt_retain);
  ESP_LOGI(TAG, "publish to: %s, msg: %s", topic, message_buf);
  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}

/* Publish led channels current duty
 * [hostname]/channel/[channel_number]
 * */
void mqtt_publish_channel_duty()
{
  if (!mqtt_enabled || !mqtt_connected)
    return;

  int msg_id;
  char topic[128];
  char message_buf[10];

  services_t * settings = get_services();

  /* Publish */
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* make topic string */
    snprintf(topic, 128, "%s/channel/%d", settings->hostname, i);

    /* make message string */
    snprintf(message_buf, 128, "%d", get_channel_duty(i));

    /* publish led status to topic */
    msg_id = esp_mqtt_client_publish(client, topic, message_buf, strlen(message_buf), settings->mqtt_qos, settings->mqtt_retain);
    ESP_LOGI(TAG, "publish to: %s, msg: %s", topic, message_buf);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

    vTaskDelay(200 / portTICK_RATE_MS);
  }
}

/* Publish led channels current state
 * [hostname]/channel/[channel_number]/state
 * */
void mqtt_publish_channel_state()
{
  if (!mqtt_enabled || !mqtt_connected)
    return;

  int msg_id;
  char topic[128];
  char message_buf[10];

  services_t * settings = get_services();

  /* Publish */
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* make topic string */
    snprintf(topic, 128, "%s/channel/%d/state", settings->hostname, i);

    /* make message string */
    snprintf(message_buf, 128, "%d", get_channel_state(i));

    /* publish led status to topic */
    msg_id = esp_mqtt_client_publish(client, topic, message_buf, strlen(message_buf), settings->mqtt_qos, settings->mqtt_retain);
    ESP_LOGI(TAG, "publish to: %s, msg: %s", topic, message_buf);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

    vTaskDelay(200 / portTICK_RATE_MS);
  }
}

/* Publish device status
 * [hostname]/status
 * */
void mqtt_publish_device_status() {
  if (!mqtt_enabled || !mqtt_connected)
    return;

  int msg_id;
  char topic[128];
  char * message_buf;
  char time_string[32];
  char channel_name[10];

  services_t * services = get_services();
  system_status_t *system_status = get_system_status();
  get_time_string((char*) &time_string);
  /* make topic string */
  snprintf(topic, 128, "%s/status", services->hostname);

  /* make message json string */
  cJSON *root = cJSON_CreateObject();

  det_time_string_since_boot((char*) &time_string);
  cJSON_AddItemToObject(root, "up_time", cJSON_CreateString(time_string));

  get_time_string((char*) &time_string);
  cJSON_AddItemToObject(root, "local_time", cJSON_CreateString(time_string));

  cJSON_AddItemToObject(root, "free_heap", cJSON_CreateNumber(system_status->free_heap));
//  cJSON_AddItemToObject(root, "vcc", cJSON_CreateNumber(get_stm_vcc_power()));
//  cJSON_AddItemToObject(root, "ntc_temperature", cJSON_CreateNumber(get_stm_ntc_temperature()));
//  cJSON_AddItemToObject(root, "board_temperature", cJSON_CreateNumber(get_stm_mcu_temperature()));
  cJSON_AddItemToObject(root, "local_ip_address", cJSON_CreateString(system_status->net_address));
  cJSON_AddItemToObject(root, "mac_address", cJSON_CreateString(system_status->mac));
  cJSON_AddItemToObject(root, "brightness", cJSON_CreateNumber(get_brightness()));

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
  msg_id = esp_mqtt_client_publish(client, topic, message_buf, strlen(message_buf), services->mqtt_qos, services->mqtt_retain);
  free(message_buf);
  ESP_LOGD(TAG, "publish to: %s", topic);
  ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);
}

mqtt_service_status_t get_mqtt_status()
{
  if (mqtt_enabled && !mqtt_connected)
    return MQTT_ENABLED_NOT_CONNECTED;
  else if (mqtt_enabled && mqtt_connected)
    return MQTT_ENABLED_CONNECTED;
  else
    return MQTT_DISABLED;
}