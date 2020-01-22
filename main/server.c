/***
** Created by Aleksey Volkov on 15.12.2019.
***/

#include <cJSON.h>
#include <esp_ota_ops.h>
#include <monitor.h>
#include <settings.h>
#include <stmdriver.h>
#include <light.h>
#include <rtc.h>
#include <mqtt.h>
#include <esp_timer.h>
#include <ota.h>
#include <main.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "lwip/api.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "string.h"
#include "webpage.h"
#include "server.h"

static const char *TAG="WEBSERVER";

#define TOCKEN_SIZE       (uint32_t)32
#define SCRATCH_BUFSIZE   (uint32_t)2048

//typedef struct rest_server_context {
//  char tocken[TOCKEN_SIZE];
//  char scratch[SCRATCH_BUFSIZE];
//} rest_server_context_t;

//static rest_server_context_t * rest_context = NULL;

/* Function to free context */
void session_free_func(void *ctx)
{
  ESP_LOGI(TAG, "Session Free Context function called");
  free(ctx);
}

/*
 * {success: true}
 * */
char * success_response_json()
{
  char *string = NULL;
  cJSON *response = cJSON_CreateObject();
  cJSON_AddTrueToObject(response, "success");

  string = cJSON_Print(response);
  if (string == NULL)
  {
    fprintf(stderr, "Failed to print monitor.\n");
  }
  cJSON_Delete(response);
  return string;
}

/* This handler gets the present value of the accumulator */
esp_err_t home_get_handler(httpd_req_t *req)
{
  /* Create session's context if not already available */
  if (! req->sess_ctx) {
    ESP_LOGI(TAG, "/ GET allocating new session");
    req->sess_ctx = malloc(sizeof(int));
    req->free_ctx = session_free_func;
    *(int *)req->sess_ctx = 0;
  }
  //*(int *)req->sess_ctx = val; //set session token
  ESP_LOGI(TAG, "/ GET handler send %d", *(int *)req->sess_ctx);

  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, (const char *)&HTML, HTML_SIZE);
  return ESP_OK;
}

/* REBOOT GET handler */
esp_err_t reboot_get_handler(httpd_req_t *req)
{
  char * response = success_response_json();
  httpd_resp_send(req, response, strlen(response));
  esp_restart();
}

/* FACTORY GET handler */
esp_err_t factory_get_handler(httpd_req_t *req)
{
  char * response = success_response_json();
  httpd_resp_send(req, response, strlen(response));

  /* Erase NVS partition */
  erase_settings();
  esp_restart();
}

/* OTA GET handler */
esp_err_t ota_get_handler(httpd_req_t *req)
{
  char * response = success_response_json();
  httpd_resp_send(req, response, strlen(response));

  /* Start ota task */
  return upgrade_firmware();
}

/* STATUS GET */
esp_err_t status_get_handler(httpd_req_t *req)
{
  /* Create session's context if not already available */
  if (! req->sess_ctx) {
    ESP_LOGI(TAG, "/status GET allocating new session");
    req->sess_ctx = malloc(sizeof(int));
    req->free_ctx = session_free_func;
    *(int *)req->sess_ctx = 0;
  }
  ESP_LOGI(TAG, "/status GET handler send %d", *(int *)req->sess_ctx);

  char * response = get_status_json();
  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}

/* SCHEDULE GET current data */
esp_err_t schedule_get_handler(httpd_req_t *req)
{
  /* Create session's context if not already available */
  if (! req->sess_ctx) {
    ESP_LOGI(TAG, "%s GET allocating new session", req->uri);
    req->sess_ctx = malloc(sizeof(int));
    req->free_ctx = session_free_func;
    *(int *)req->sess_ctx = 0;
  }
  ESP_LOGI(TAG, "%s GET handler send %d", req->uri, *(int *)req->sess_ctx);

  char * response = get_schedule_json();
  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}

/* SETTINGS GET current data */
esp_err_t settings_get_handler(httpd_req_t *req)
{
  /* Create session's context if not already available */
  if (! req->sess_ctx) {
    ESP_LOGI(TAG, "%s GET allocating new session", req->uri);
    req->sess_ctx = malloc(sizeof(int));
    req->free_ctx = session_free_func;
    *(int *)req->sess_ctx = 0;
  }
  ESP_LOGI(TAG, "%s GET handler send %d", req->uri, *(int *)req->sess_ctx);

  char * response = get_settings_json();
  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}

/* LIGHT POST new data */
static esp_err_t light_post_handler(httpd_req_t *req)
{
  int total_len = req->content_len;
  int cur_len = 0;
  char *buf = malloc(req->content_len + 1);
  int received = 0;
  if (total_len >= SCRATCH_BUFSIZE)
  {
    /* Respond with 500 Internal Server Error */
    return ESP_FAIL;
  }
  while (cur_len < total_len)
  {
    received = httpd_req_recv(req, buf + cur_len, total_len);
    if (received <= 0)
    {
      /* Respond with 500 Internal Server Error */
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  cJSON *root = cJSON_Parse(buf);

  /* channels duty --------------------------- */
  uint8_t channel_id = 0;
  uint8_t target_duty[MAX_LED_CHANNELS] = {0};
  uint8_t target_brightness = 0;
  cJSON * channel_item;
  cJSON * channels = cJSON_GetObjectItem(root, "channels");
  if (cJSON_IsArray(channels)) {
    cJSON_ArrayForEach(channel_item, channels)
    {
      if (cJSON_IsNumber(channel_item))
      {
        target_duty[channel_id] = channel_item->valueint;
        ESP_LOGI(TAG, "light_post_handler channel %d new duty %d", channel_id, channel_item->valueint);

      }
      channel_id++;
    }
  }

  /* channels brightness --------------------- */
  cJSON * brightness = cJSON_GetObjectItem(root, "brightness");
  if (cJSON_IsNumber(brightness))
  {
    target_brightness = brightness->valueint;
    ESP_LOGI(TAG, "light_post_handler new brightness %d", brightness->valueint);
  }

  set_light(target_duty, target_brightness);

  cJSON_Delete(root);

  char * response = success_response_json();
  httpd_resp_send(req, response, strlen(response));
  free (buf);
  return ESP_OK;
}

/* SCHEDULE POST new data */
static esp_err_t schedule_post_handler(httpd_req_t *req)
{
  int total_len = req->content_len;
  int cur_len = 0;
  char *buf = malloc(req->content_len + 1);
  int received = 0;
  if (total_len >= SCRATCH_BUFSIZE) {
    /* Respond with 500 Internal Server Error */
    return ESP_FAIL;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, buf + cur_len, total_len);
    if (received <= 0) {
      /* Respond with 500 Internal Server Error */
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  cJSON *root = cJSON_Parse(buf);

  /* Reset schedule cache */
  for (uint8_t i = 0; i < MAX_SCHEDULE; i++) {
    schedule_t * schedule_config = get_schedule_config(i);
    schedule_config->active = false;
  }

  /* channels schedule ----------------------- */
  uint8_t id = 0;
  cJSON * schedule_item;
  cJSON * schedule = cJSON_GetObjectItem(root, "schedule");
  cJSON_ArrayForEach(schedule_item, schedule)
  {
    schedule_t * schedule_config = get_schedule_config(id);

    cJSON * time_hour = cJSON_GetObjectItem(schedule_item, "time_hour");
    cJSON * time_minute = cJSON_GetObjectItem(schedule_item, "time_minute");
    cJSON * brightness = cJSON_GetObjectItem(schedule_item, "brightness");

    schedule_config->time_hour = time_hour->valueint;
    schedule_config->time_minute = time_minute->valueint;
    schedule_config->brightness = brightness->valueint;

    uint8_t channel_id = 0;
    cJSON * duty_item;
    cJSON * duty = cJSON_GetObjectItem(schedule_item, "duty");
    cJSON_ArrayForEach(duty_item, duty)
    {
      schedule_config->duty[channel_id] = duty_item->valueint;
      channel_id++;
    }
    schedule_config->active = true;
    id++;
  }

  char * debug_string = NULL;
  debug_string = cJSON_Print(root);
  if (debug_string == NULL) {
    fprintf(stderr, "Failed to print time.\n");
  } else {
    ESP_LOGI(TAG, "JSON parse schedule: %s", debug_string);
  }

  cJSON_Delete(root);

  /* Store New Schedule in NVS */
  set_schedule();

  char * response = success_response_json();
  httpd_resp_send(req, response, strlen(response));
  free(buf);
  return ESP_OK;
}

/* SETTINGS POST new data */
static esp_err_t settings_post_handler(httpd_req_t *req)
{
  int total_len = req->content_len;
  int cur_len = 0;
  char *buf = malloc(req->content_len + 1);
  int received = 0;
  if (total_len >= SCRATCH_BUFSIZE) {
    /* Respond with 500 Internal Server Error */
    return ESP_FAIL;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, buf + cur_len, total_len);
    if (received <= 0) {
      /* Respond with 500 Internal Server Error */
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  cJSON *root = cJSON_Parse(buf);

  /* led channels config --------------------- */
  cJSON * led_item;
  cJSON * led_channels = cJSON_GetObjectItem(root, "leds");
  if (cJSON_IsArray(led_channels))
  {
    cJSON_ArrayForEach(led_item, led_channels)
    {
      cJSON * id = cJSON_GetObjectItem(led_item, "id");

      led_t * led_config = get_led_config(id->valueint);

      cJSON * color = cJSON_GetObjectItem(led_item, "color");
      if (cJSON_IsString(color) && (color->valuestring != NULL))
      {
        strlcpy(led_config->color, color->valuestring, 8);
      }

      cJSON * power = cJSON_GetObjectItem(led_item, "power");
      cJSON * state = cJSON_GetObjectItem(led_item, "state");
      led_config->power = power->valueint;
      led_config->state = state->valueint;
    }

    char * debug_string = NULL;
    debug_string = cJSON_Print(led_channels);
    if (debug_string == NULL) {
      fprintf(stderr, "Failed to print led_channels.\n");
    } else {
      ESP_LOGI(TAG, "JSON parse led_channels: %s", debug_string);
    }

    /* Save Led NVS */
    set_led();
  }

  /* networks config ------------------------- */
  cJSON * network_item;
  cJSON * networks = cJSON_GetObjectItem(root, "networks");

  if (cJSON_IsArray(networks) && cJSON_GetArraySize(networks) > 0) {
    /* Reset network cache */
    for (uint8_t i = 0; i < MAX_NETWORKS; i++) {
      network_t *network_config = get_network_config(i);
      network_config->active = false;
    }

    cJSON_ArrayForEach(network_item, networks) {

      cJSON * id = cJSON_GetObjectItem(network_item, "id");

      network_t *network_config = get_network_config(id->valueint);

      cJSON *ssid = cJSON_GetObjectItem(network_item, "ssid");
      if (cJSON_IsString(ssid) && (ssid->valuestring != NULL)) {
        strlcpy(network_config->ssid, ssid->valuestring, 32);
      }

      cJSON *password = cJSON_GetObjectItem(network_item, "password");
      if (cJSON_IsString(password) && (password->valuestring != NULL)) {
        strlcpy(network_config->password, password->valuestring, 64);
      }

      cJSON *ip_address = cJSON_GetObjectItem(network_item, "ip_address");
      if (cJSON_IsString(ip_address) && (ip_address->valuestring != NULL)) {
        string_to_ip(ip_address->valuestring, network_config->ip_address);
      }

      cJSON *mask = cJSON_GetObjectItem(network_item, "mask");
      if (cJSON_IsString(mask) && (mask->valuestring != NULL)) {
        string_to_ip(mask->valuestring, network_config->mask);
      }

      cJSON *gateway = cJSON_GetObjectItem(network_item, "gateway");
      if (cJSON_IsString(gateway) && (gateway->valuestring != NULL)) {
        string_to_ip(gateway->valuestring, network_config->gateway);
      }

      cJSON *dns = cJSON_GetObjectItem(network_item, "dns");
      if (cJSON_IsString(dns) && (dns->valuestring != NULL)) {
        string_to_ip(dns->valuestring, network_config->dns);
      }

      cJSON *dhcp = cJSON_GetObjectItem(network_item, "dhcp");
      if (cJSON_IsTrue(dhcp)) {
        network_config->dhcp = true;
      } else {
        network_config->dhcp = false;
      }

      network_config->active = true;
    }

    char * debug_string = NULL;
    debug_string = cJSON_Print(networks);
    if (debug_string == NULL) {
      fprintf(stderr, "Failed to print networks.\n");
    }

    /* Save Network NVS */
    set_network();
  }

  /* services config ------------------------- */
  cJSON * services = cJSON_GetObjectItem(root, "services");
  if (cJSON_IsObject(services))
  {
    services_t * service_config = get_service_config();

    cJSON *hostname = cJSON_GetObjectItem(services, "hostname");
    if (cJSON_IsString(hostname) && (hostname->valuestring != NULL)) {
      strlcpy(service_config->hostname, hostname->valuestring, 20);
    }

    cJSON *ota_url = cJSON_GetObjectItem(services, "ota_url");
    if (cJSON_IsString(ota_url) && (ota_url->valuestring != NULL)) {
      strlcpy(service_config->ota_url, ota_url->valuestring, 64);
    }

    cJSON *ntp_server = cJSON_GetObjectItem(services, "ntp_server");
    if (cJSON_IsString(ntp_server) && (hostname->valuestring != NULL)) {
      strlcpy(service_config->ntp_server, ntp_server->valuestring, 20);
    }

    cJSON *utc_offset = cJSON_GetObjectItem(services, "utc_offset");
    service_config->utc_offset = utc_offset->valueint;

    cJSON *ntp_dst = cJSON_GetObjectItem(services, "ntp_dst");
    if (cJSON_IsTrue(ntp_dst)) {
      service_config->ntp_dst = true;
    } else {
      service_config->ntp_dst = false;
    }

    cJSON *mqtt_ip_address = cJSON_GetObjectItem(services, "mqtt_ip_address");
    if (cJSON_IsString(mqtt_ip_address) && (mqtt_ip_address->valuestring != NULL)) {
      string_to_ip(mqtt_ip_address->valuestring, service_config->mqtt_ip_address);
    }

    cJSON *mqtt_port = cJSON_GetObjectItem(services, "mqtt_port");
    if (cJSON_IsNumber(mqtt_port)) {
      service_config->mqtt_port = mqtt_port->valueint;
    }

    cJSON *mqtt_user = cJSON_GetObjectItem(services, "mqtt_user");
    if (cJSON_IsString(mqtt_user) && (mqtt_user->valuestring != NULL)) {
      strlcpy(service_config->mqtt_user, mqtt_user->valuestring, 16);
    }

    cJSON *mqtt_password = cJSON_GetObjectItem(services, "mqtt_password");
    if (cJSON_IsString(mqtt_password) && (mqtt_password->valuestring != NULL)) {
      strlcpy(service_config->mqtt_password, mqtt_password->valuestring, 16);
    }

    cJSON *mqtt_qos = cJSON_GetObjectItem(services, "mqtt_qos");
    service_config->mqtt_qos = mqtt_qos->valueint;

    cJSON *enable_ntp = cJSON_GetObjectItem(services, "enable_ntp");
    if (cJSON_IsTrue(enable_ntp)) {
      service_config->enable_ntp = true;
    } else {
      service_config->enable_ntp = false;
    }

    cJSON *enable_mqtt = cJSON_GetObjectItem(services, "enable_mqtt");
    if (cJSON_IsTrue(enable_mqtt)) {
      service_config->enable_mqtt = true;
    } else {
      service_config->enable_mqtt = false;
    }

    char * debug_string = NULL;
    debug_string = cJSON_Print(services);
    if (debug_string == NULL) {
      fprintf(stderr, "Failed to print services.\n");
    } else {
      ESP_LOGI(TAG, "JSON parse services: %s", debug_string);
    }

    /* Save Services NVS */
    set_service();
  }

  /* time config ----------------------------- */
  cJSON * time = cJSON_GetObjectItem(root, "time");
  if (cJSON_IsObject(time))
  {
    cJSON * year = cJSON_GetObjectItem(time, "year");
    cJSON * month = cJSON_GetObjectItem(time, "month");
    cJSON * day = cJSON_GetObjectItem(time, "day");
    cJSON * weekday = cJSON_GetObjectItem(time, "weekday");
    cJSON * hour = cJSON_GetObjectItem(time, "hour");
    cJSON * minute = cJSON_GetObjectItem(time, "minute");
    cJSON * second = cJSON_GetObjectItem(time, "second");

    stm_datetime_t datetime;
    datetime.year = year->valueint;
    datetime.month = month->valueint;
    datetime.day = day->valueint;
    datetime.weekday = weekday->valueint;
    datetime.hour = hour->valueint;
    datetime.minute = minute->valueint;
    datetime.second = second->valueint;

    set_stm_rtc(&datetime);

    char * debug_string = NULL;
    debug_string = cJSON_Print(time);
    if (debug_string == NULL) {
      fprintf(stderr, "Failed to print time.\n");
    } else {
      ESP_LOGI(TAG, "JSON parse time: %s", debug_string);
    }
  }

  cJSON_Delete(root);

  char * response = success_response_json();
  httpd_resp_send(req, response, strlen(response));
  free(buf);
  return ESP_OK;
}

/* This handler useful for test, enable CORS access */
esp_err_t options_handler(httpd_req_t *req)
{
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}

httpd_handle_t start_webserver(void)
{
  httpd_handle_t server;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16;

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

  // Start the httpd server
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");

    /* Allow CORS */
    httpd_uri_t global_options = {
        .uri      = "/*",
        .method   = HTTP_OPTIONS,
        .handler  = options_handler,
        .user_ctx = NULL
    };

    /* Load Vue.js App */
    httpd_uri_t get_home = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = home_get_handler,
        .user_ctx = NULL
    };

    /* Reboot device */
    httpd_uri_t get_reboot = {
        .uri      = "/reboot",
        .method   = HTTP_GET,
        .handler  = reboot_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t get_factory = {
        .uri      = "/factory",
        .method   = HTTP_GET,
        .handler  = factory_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t get_ota = {
        .uri      = "/update",
        .method   = HTTP_GET,
        .handler  = ota_get_handler,
        .user_ctx = NULL
    };

    /* Send current device status */
    httpd_uri_t get_status = {
        .uri      = "/status",
        .method   = HTTP_GET,
        .handler  = status_get_handler,
        .user_ctx = NULL
    };

    /* POST Light */
    httpd_uri_t post_light = {
        .uri      = "/api/light",
        .method   = HTTP_POST,
        .handler  = light_post_handler,
        .user_ctx = NULL
    };

    /* Get Schedule */
    httpd_uri_t get_schedule = {
        .uri      = "/api/schedule",
        .method   = HTTP_GET,
        .handler  = schedule_get_handler,
        .user_ctx = NULL
    };

    /* POST Schedule */
    httpd_uri_t post_schedule = {
        .uri      = "/api/schedule",
        .method   = HTTP_POST,
        .handler  = schedule_post_handler,
        .user_ctx = NULL
    };

    /* Get current device settings */
    httpd_uri_t get_settings = {
        .uri      = "/api/settings",
        .method   = HTTP_GET,
        .handler  = settings_get_handler,
        .user_ctx = NULL
    };

    /* POST current device settings */
    httpd_uri_t post_settings = {
        .uri      = "/api/settings",
        .method   = HTTP_POST,
        .handler  = settings_post_handler,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(server, &global_options);
    httpd_register_uri_handler(server, &get_reboot);
    httpd_register_uri_handler(server, &get_factory);
    httpd_register_uri_handler(server, &get_ota);
    httpd_register_uri_handler(server, &get_home);
    httpd_register_uri_handler(server, &get_status);
    httpd_register_uri_handler(server, &post_light);
    httpd_register_uri_handler(server, &get_schedule);
    httpd_register_uri_handler(server, &post_schedule);
    httpd_register_uri_handler(server, &get_settings);
    httpd_register_uri_handler(server, &post_settings);

    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

/* Stop the httpd server */
void stop_webserver(httpd_handle_t server)
{
  httpd_stop(server);
  server = NULL;
}

/* JSON -------------------------------------- */

/* http://device.name/status
    status: {
          upTime: '1 day 23:40',
          localTime: '12:00',
          chipId: 0,
          freeHeap: 0,
          vcc: 0,
          wifiMode: 0,
          ipAddress: '',
          macAddress: '00:00:00:00:00:00',
          mqttService: '',
          ntpService: '',
          channels: [50, 100, 100, 20, 20],
          brightness: 90
    }
*/
char * get_status_json()
{
  char * string = NULL;
  char time_string[32];
  system_status_t *system_status = get_system_status();
  services_t * services = get_service_config();

  cJSON * root = cJSON_CreateObject();
  cJSON * status = cJSON_CreateObject();

  det_time_string_since_boot((char*) &time_string);
  cJSON_AddItemToObject(status, "upTime", cJSON_CreateString(time_string));

  get_time_string((char*) &time_string);
  cJSON_AddItemToObject(status, "localTime", cJSON_CreateString(time_string));

  cJSON_AddItemToObject(status, "chipId", cJSON_CreateNumber(system_status->revision));
  cJSON_AddItemToObject(status, "freeHeap", cJSON_CreateNumber(system_status->free_heap));
  cJSON_AddItemToObject(status, "vcc", cJSON_CreateNumber(get_stm_vcc_power()));
  cJSON_AddItemToObject(status, "ntc_temperature", cJSON_CreateNumber(get_stm_ntc_temperature()));
  cJSON_AddItemToObject(status, "board_temperature", cJSON_CreateNumber(get_stm_mcu_temperature()));
  cJSON_AddItemToObject(status, "wifiMode", cJSON_CreateString(system_status->wifi_mode));
  cJSON_AddItemToObject(status, "ipAddress", cJSON_CreateString(system_status->net_address));
  cJSON_AddItemToObject(status, "macAddress", cJSON_CreateString(system_status->mac));

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
  cJSON_AddItemToObject(status, "mqttService", mqtt_status);

  /* NTP Status */
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
  cJSON_AddItemToObject(status, "ntpService", ntp_status);
  cJSON_AddItemToObject(status, "brightness", cJSON_CreateNumber(get_brightness()));

  /* Channels duty */
  cJSON * channels = cJSON_CreateArray();
  for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
    cJSON_AddItemToArray(channels, cJSON_CreateNumber(get_channel_duty(i)));
  }

  cJSON_AddItemToObject(status, "channels", channels);
  cJSON_AddItemToObject(root, "status", status);

  string = cJSON_Print(root);
  if (string == NULL)
  {
    fprintf(stderr, "Failed to print status json.\n");
  }

  cJSON_Delete(root);
  return string;
}

/* http://device.name/api/schedule
  schedule: [
  {
    time_hour: 9,
    time_minute: 0,
    brightness: 50,
    duty: [
      10, 20, 10, 20, 20
    ]
  }]
*/
char * get_schedule_json()
{
  char *string = NULL;

  cJSON *root = cJSON_CreateObject();

  /* channels schedule */
  cJSON *schedule = cJSON_CreateArray();
  for (int i = 0; i < MAX_SCHEDULE; ++i) {
    schedule_t * schedule_config = get_schedule_config(i);
    if (schedule_config->active) {
      cJSON *schedule_item = cJSON_CreateObject();
      cJSON_AddItemToObject(schedule_item, "time_hour", cJSON_CreateNumber(schedule_config->time_hour));
      cJSON_AddItemToObject(schedule_item, "time_minute", cJSON_CreateNumber(schedule_config->time_minute));
      cJSON_AddItemToObject(schedule_item, "brightness", cJSON_CreateNumber(schedule_config->brightness));

      cJSON *schedule_item_duty = cJSON_CreateArray();
      for (int j = 0; j < MAX_LED_CHANNELS; ++j) {
        cJSON_AddItemToArray(schedule_item_duty, cJSON_CreateNumber(schedule_config->duty[j]));
      }
      cJSON_AddItemToObject(schedule_item, "duty", schedule_item_duty);
      cJSON_AddItemToArray(schedule, schedule_item);
    }
  }
  cJSON_AddItemToObject(root, "schedule", schedule);

  string = cJSON_Print(root);
  if (string == NULL)
  {
    fprintf(stderr, "Failed to print schedule json.\n");
  }

  cJSON_Delete(root);
  return string;
}

/* http://device.name/api/settings
  settings: {
    leds: [
    {
      id: 0,
      color: '#DDEFFF',
      power: 50,
      state: 1
    }],
    networks: [
    {
      id: 0,
      ssid: 'Best WiFi',
      password: '',
      ip_address: '192.168.1.100',
      mask: '255.255.255.0',
      gateway: '192.168.1.1',
      dns: '192.168.1.1',
      dhcp: false
    }],
    services:
    {
      hostname: 'test',
      ntp_server_name: '',
      utc_offset_minutes: 0,
      ntp_dst: true,
      mqtt_ip_address: '',
      mqtt_port: '',
      mqtt_user: '',
      mqtt_password: '',
      mqtt_qos: 0,
      enable_ntp_service: false,
      enable_mqtt_service: false
    },
    time:
    {
      year: 20,
      month: 10,
      weekday: 1,
      day: 10,
      hour: 12,
      minute: 1,
      second: 1,
      dst: 0,
      utc: 1
    }}
*/
char * get_settings_json()
{
  char *string = NULL;

  cJSON * root = cJSON_CreateObject();

  /* led channels config */
  cJSON *led_channels = cJSON_CreateArray();
  for (int i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    led_t * led_config = get_led_config(i);

    cJSON * led_item = cJSON_CreateObject();
    cJSON_AddItemToObject(led_item, "id", cJSON_CreateNumber(led_config->id));
    cJSON_AddItemToObject(led_item, "color", cJSON_CreateString(led_config->color));
    cJSON_AddItemToObject(led_item, "power", cJSON_CreateNumber(led_config->power));
    cJSON_AddItemToObject(led_item, "state", cJSON_CreateNumber(led_config->state));

    cJSON_AddItemToArray(led_channels, led_item);
  }
  cJSON_AddItemToObject(root, "leds", led_channels);

  /* networks config */
  cJSON * networks = cJSON_CreateArray();
  for (int j = 0; j < MAX_NETWORKS; ++j) {
    network_t * network_config = get_network_config(j);
    if (network_config->active)
    {
      cJSON * network_item = cJSON_CreateObject();
      cJSON_AddItemToObject(network_item, "id", cJSON_CreateNumber(j));
      cJSON_AddItemToObject(network_item, "ssid", cJSON_CreateString(network_config->ssid));
      cJSON_AddItemToObject(network_item, "password", cJSON_CreateString(network_config->password));

      char net_buff[16];
      ip_to_string(network_config->ip_address, net_buff);
      cJSON_AddItemToObject(network_item, "ip_address", cJSON_CreateString((char*)&net_buff));

      ip_to_string(network_config->mask, net_buff);
      cJSON_AddItemToObject(network_item, "mask", cJSON_CreateString((char*)&net_buff));

      ip_to_string(network_config->gateway, net_buff);
      cJSON_AddItemToObject(network_item, "gateway", cJSON_CreateString((char*)&net_buff));

      ip_to_string(network_config->dns, net_buff);
      cJSON_AddItemToObject(network_item, "dns", cJSON_CreateString((char*)&net_buff));
      cJSON_AddItemToObject(network_item, "dhcp", cJSON_CreateBool(network_config->dhcp));

      cJSON_AddItemToArray(networks, network_item);
    }
  }
  cJSON_AddItemToObject(root, "networks", networks);

  /* services config */
  cJSON * services = cJSON_CreateObject();
  services_t * service_config = get_service_config();
  cJSON_AddItemToObject(services, "hostname", cJSON_CreateString(service_config->hostname));
  cJSON_AddItemToObject(services, "ota_url", cJSON_CreateString(service_config->ota_url));
  cJSON_AddItemToObject(services, "ntp_server", cJSON_CreateString(service_config->ntp_server));
  cJSON_AddItemToObject(services, "utc_offset", cJSON_CreateNumber(service_config->utc_offset));
  cJSON_AddItemToObject(services, "ntp_dst", cJSON_CreateBool(service_config->ntp_dst));

  char net_buff[16];
  ip_to_string(service_config->mqtt_ip_address, net_buff);
  cJSON_AddItemToObject(services, "mqtt_ip_address", cJSON_CreateString(net_buff));
  cJSON_AddItemToObject(services, "mqtt_port", cJSON_CreateNumber(service_config->mqtt_port));
  cJSON_AddItemToObject(services, "mqtt_user", cJSON_CreateString(service_config->mqtt_user));
  cJSON_AddItemToObject(services, "mqtt_password", cJSON_CreateString(service_config->mqtt_password));
  cJSON_AddItemToObject(services, "mqtt_qos", cJSON_CreateNumber(service_config->mqtt_qos));
  cJSON_AddItemToObject(services, "enable_ntp", cJSON_CreateBool(service_config->enable_ntp));
  cJSON_AddItemToObject(services, "enable_mqtt", cJSON_CreateBool(service_config->enable_mqtt));

  cJSON_AddItemToObject(root, "services", services);

  /* time config */
  stm_datetime_t datetime;
  get_stm_rtc(&datetime);

  cJSON * time = cJSON_CreateObject();
  cJSON_AddItemToObject(time, "year", cJSON_CreateNumber(datetime.year));
  cJSON_AddItemToObject(time, "month", cJSON_CreateNumber(datetime.month));
  cJSON_AddItemToObject(time, "weekday", cJSON_CreateNumber(datetime.weekday));
  cJSON_AddItemToObject(time, "day", cJSON_CreateNumber(datetime.day));
  cJSON_AddItemToObject(time, "hour", cJSON_CreateNumber(datetime.hour));
  cJSON_AddItemToObject(time, "minute", cJSON_CreateNumber(datetime.minute));
  cJSON_AddItemToObject(time, "second", cJSON_CreateNumber(datetime.second));

  cJSON_AddItemToObject(root, "time", time);

  string = cJSON_Print(root);
  if (string == NULL)
  {
    fprintf(stderr, "Failed to print settings json.\n");
  }

  cJSON_Delete(root);
  return string;
}