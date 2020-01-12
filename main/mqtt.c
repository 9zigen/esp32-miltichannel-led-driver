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
#include <stmdriver.h>
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

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

#define DEFAULT_ROOT "LED_DRIVER"  /* MQTT default root topic */

static const char *TAG = "MQTT";

esp_mqtt_client_handle_t client;
uint8_t mqtt_enabled = 0;
uint8_t mqtt_connected = 0;
TimerHandle_t xTimerStatus;

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
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      mqtt_connected = 1;
      mqtt_subscribe();

      /* publish current info */
      mqtt_publish_device_status();
      mqtt_publish_channel_duty();
      mqtt_publish_brightness();
      mqtt_publish_channel_state();

      /* start device info timer */
      xTimerStart( xTimerStatus, 100 );
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
      mqtt_connected = 0;
      xTimerStop( xTimerStatus, 100 );
      break;
    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//      msg_id = esp_mqtt_client_publish(client, CONFIG_EMITTER_CHANNEL_KEY"/topic/", "data", 0, 0, 0);
//      ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
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
  services_t * settings = get_service_config();

  char mqtt_address_buff[64];
  char ip_address[16];
  ip_to_string(settings->mqtt_ip_address, ip_address);
  snprintf(mqtt_address_buff, 64, "mqtt://%s:%d", ip_address, settings->mqtt_port);

  ESP_LOGI(TAG, "enabled: %d | address: %s | user:%s | password:%s", settings->enable_mqtt, mqtt_address_buff,
           settings->mqtt_user, settings->mqtt_password);

  if (settings->enable_mqtt && strlen(mqtt_address_buff) > 20)
  {
    esp_mqtt_client_config_t mqtt_cfg;
    mqtt_cfg.uri = (const char*) &mqtt_address_buff;
    mqtt_cfg.event_handle = mqtt_event_handler;

    if (strlen(settings->mqtt_user) > 3) {
      mqtt_cfg.username = (const char*) &settings->mqtt_user;
    }

    if (strlen(settings->mqtt_password) > 1) {
      mqtt_cfg.password = (const char*) &settings->mqtt_password;
    }

    mqtt_enabled = 1;

    /* Create device status publish timer, 1 min period */
    xTimerStatus = xTimerCreate("TimerDeviceStatus",60 * 1000 / portTICK_RATE_MS, pdTRUE, NULL, vTimerCallback);

    ESP_LOGI(TAG, "[APP] free memory before start MQTT client: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
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

  char topic[128];
  
  services_t * settings = get_service_config();
  
  /* check brightness topic */
  snprintf(topic, 128, "%s/brightness/set", settings->hostname);

  /* Brightness received */
  if (strncmp(incoming_event->topic, topic, 128) == 0) {
    uint8_t brightness = atoi(incoming_event->data) & 0xff;

    ESP_LOGI(TAG,"set brightness: %d\n", brightness);
    set_brightness(brightness);

    /* update topics */
    mqtt_publish_brightness();
    return;
  }

  /* check switch or set command */
  uint32_t channel = 0;
  char command[7];
  char scan[128];

  /* "sscanf" template example: LED_11324571/channel/%u/%6s */
  snprintf(scan, 128, "%s/channel/%%u/%%6s", settings->hostname);
  sscanf(topic, scan, &channel, command);

  ESP_LOGI(TAG,"command: %s payload: %s\n", command, incoming_event->data);

  if (strncmp(command, "set", 3) == 0) {
    /* Set command */
    uint8_t duty = atoi(incoming_event->data) & 0xff;
    set_channel_duty(channel, duty);
    ESP_LOGI(TAG,"set duty: %u channel: %u\n", duty, channel);

    /* update topics */
    mqtt_publish_channel_duty();

  } else if (strncmp(command, "switch", 6) == 0) {
    /* switch command */
    uint8_t state = atoi(incoming_event->data) & 0xff;
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

  services_t * settings = get_service_config();

  /* Subscribe to Set Duty topic
   * [hostname]/channel/[channel_number]/set
   * */
  for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
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
  for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
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

  services_t * settings = get_service_config();

  /* make topic string */
  snprintf(topic, 128, "%s/brightness", settings->hostname);

  /* make message string */
  snprintf(message_buf, 128, "%d", get_brightness());

  /* publish led status to topic QoS 0, ToDo Retain */
  msg_id = esp_mqtt_client_publish(client, topic, message_buf, 0, settings->mqtt_qos, 0);
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

  services_t * settings = get_service_config();

  /* Publish */
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* make topic string */
    snprintf(topic, 128, "%s/channel/%d", settings->hostname, i);

    /* make message string */
    snprintf(message_buf, 128, "%d", get_channel_duty(i));

    /* publish led status to topic QoS 0, ToDo Retain */
    msg_id = esp_mqtt_client_publish(client, topic, message_buf, 0, settings->mqtt_qos, 0);
    ESP_LOGI(TAG, "publish to: %s, msg: %s", topic, message_buf);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
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

  services_t * settings = get_service_config();

  /* Publish */
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* make topic string */
    snprintf(topic, 128, "%s/channel/%d/state", settings->hostname, i);

    /* make message string */
    snprintf(message_buf, 128, "%d", get_channel_state(i));

    /* publish led status to topic QoS 0, ToDo Retain */
    msg_id = esp_mqtt_client_publish(client, topic, message_buf, 0, settings->mqtt_qos, 0);
    ESP_LOGI(TAG, "publish to: %s, msg: %s", topic, message_buf);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
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

  services_t * services = get_service_config();
  system_status_t *system_status = get_system_status();
  get_time_string((char*) &time_string);
  /* make topic string */
  snprintf(topic, 128, "%s/status", services->hostname);

  /* make message json string */
  cJSON *root = cJSON_CreateObject();

  det_time_string_since_boot((char*) &time_string);
  cJSON_AddItemToObject(root, "upTime", cJSON_CreateString(time_string));

  get_time_string((char*) &time_string);
  cJSON_AddItemToObject(root, "localTime", cJSON_CreateString(time_string));

  cJSON_AddItemToObject(root, "freeHeap", cJSON_CreateNumber(system_status->free_heap));
  cJSON_AddItemToObject(root, "vcc", cJSON_CreateNumber(get_stm_vcc_power()));
  cJSON_AddItemToObject(root, "temperature", cJSON_CreateNumber(get_stm_ntc_temperature()));
  cJSON_AddItemToObject(root, "wifiMode", cJSON_CreateString(system_status->wifi_mode));
  cJSON_AddItemToObject(root, "ipAddress", cJSON_CreateString(system_status->net_address));
  cJSON_AddItemToObject(root, "macAddress", cJSON_CreateString(system_status->mac));

  /* MQTT status */
  cJSON * mqtt_status = cJSON_CreateObject();
  switch (get_mqtt_status()) {
    case MQTT_DISABLED:
      cJSON_AddFalseToObject(mqtt_status, "enabled");
      cJSON_AddFalseToObject(mqtt_status, "connected");
      break;

    case MQTT_ENABLED_NOT_CONNECTED:
      cJSON_AddTrueToObject(mqtt_status, "enabled");
      cJSON_AddFalseToObject(mqtt_status, "connected");
      break;

    case MQTT_ENABLED_CONNECTED:
      cJSON_AddTrueToObject(mqtt_status, "enabled");
      cJSON_AddTrueToObject(mqtt_status, "connected");
      break;
  }
  cJSON_AddItemToObject(root, "mqttService", mqtt_status);

  cJSON * ntp_status = cJSON_CreateObject();
  if (services->enable_ntp) {
    cJSON_AddTrueToObject(ntp_status, "enabled");
  } else {
    cJSON_AddFalseToObject(mqtt_status, "enabled");
  }
  if (get_ntp_sync_status()) {
    cJSON_AddTrueToObject(ntp_status, "sync");
  } else {
    cJSON_AddFalseToObject(mqtt_status, "sync");
  }
  cJSON_AddItemToObject(root, "ntpService", ntp_status);
  cJSON_AddItemToObject(root, "brightness", cJSON_CreateNumber(get_brightness()));

  /* Channels duty */
  cJSON *channels = cJSON_CreateArray();
  for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
    cJSON_AddItemToArray(channels, cJSON_CreateNumber(get_channel_duty(i)));
  }
  cJSON_AddItemToObject(root, "channels", channels);
  message_buf = cJSON_Print(root);
  if (message_buf == NULL)
  {
    ESP_LOGE(TAG, "failed to print status json.");
  }

  /* publish led status to topic QoS 0, ToDo Retain */
  msg_id = esp_mqtt_client_publish(client, topic, message_buf, 0, services->mqtt_qos, 0);
  ESP_LOGI(TAG, "publish to: %s, msg: %s", topic, message_buf);
  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
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