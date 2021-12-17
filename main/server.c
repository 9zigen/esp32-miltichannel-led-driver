/***
** Created by Aleksey Volkov on 15.12.2019.
***/

#include <cJSON.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "string.h"
#include "lwip/api.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "board.h"
#include "ota.h"
#include "webpage.h"
#include "settings.h"
#include <monitor.h>
#include "light.h"
#include "main.h"
#include "mcp7940.h"
#include "adc.h"
#include <fanspeed.h>
#include "rtc.h"
#include "mqtt.h"
#include "auth.h"
#include "server.h"

static const char *TAG="WEBSERVER";
static httpd_handle_t server = NULL;
static char auth_token[65];

#define TOKEN_SIZE        (uint32_t)32
#define SCRATCH_BUFSIZE   (uint32_t)4096
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]   asm("_binary_favicon_ico_end");

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

static esp_err_t validate_request(httpd_req_t *req)
{
  char*  buf;
  size_t buf_len;

  /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
  buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
  if (buf_len > 1) {
    buf = malloc(buf_len);
    /* Copy null terminated value string into buffer */
    if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
      ESP_LOGD(TAG, "Found header => Authorization: %s", buf);
      if(strncmp(auth_token, buf, strlen(auth_token)) == 0) {
        ESP_LOGD(TAG, "Authorization: success");
        return ESP_OK;
      }
    }
    free(buf);
  }
  return ESP_ERR_HTTPD_INVALID_REQ;
}

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
  const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
  httpd_resp_set_type(req, "image/x-icon");
  httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
  return ESP_OK;
}

/* This handler gets the present value of the accumulator */
esp_err_t home_get_handler(httpd_req_t *req)
{
  /* Create session's context if not already available */
//  if (! req->sess_ctx) {
//    ESP_LOGI(TAG, "/ GET allocating new session");
//    req->sess_ctx = malloc(sizeof(int));
//    req->free_ctx = session_free_func;
//    *(int *)req->sess_ctx = 0;
//  }
//  //*(int *)req->sess_ctx = val; //set session token
//  ESP_LOGI(TAG, "/ GET handler send %d", *(int *)req->sess_ctx);

  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, (const char *)&HTML, HTML_SIZE);
  return ESP_OK;
}

/* REBOOT GET handler */
esp_err_t reboot_get_handler(httpd_req_t *req)
{
  /* Auth token validation */
  if (validate_request(req) == ESP_OK)
  {
    char * response = success_response_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);

    /* Restart Controller */
    esp_restart();
  } else {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  }
  return ESP_OK;
}

/* FACTORY GET handler */
esp_err_t factory_get_handler(httpd_req_t *req)
{
  /* Auth token validation */
  if (validate_request(req) == ESP_OK)
  {
    char * response = success_response_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);

    /* Erase NVS partition */
    erase_settings();
    esp_restart();
  } else {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  }
  return ESP_OK;
}

/* OTA GET handler */
esp_err_t ota_get_handler(httpd_req_t *req)
{
  /* Auth token validation */
  if (validate_request(req) == ESP_OK)
  {
    char * response = success_response_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);

    /* Start ota task */
    upgrade_firmware();
  } else {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  }

  return ESP_OK;
}

/* OTA POST handler */
esp_err_t upload_post_handler(httpd_req_t *req)
{
  char buf[1024];
  esp_ota_handle_t ota_handle;
  int remaining = req->content_len;
  bool header_skiped = false;

  const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
  ESP_ERROR_CHECK(esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle));

  while (remaining > 0) {
    int recv_len = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));

    ESP_LOGD(TAG, "recv_len %d", recv_len);

    // Timeout Error: Just retry
    if (recv_len == HTTPD_SOCK_ERR_TIMEOUT) {
      continue;

      // Serious Error: Abort OTA
    } else if (recv_len <= 0) {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Protocol Error");
      return ESP_FAIL;
    }

    if (!header_skiped) {
      header_skiped = true;

      // Lets find out where the actual data staers after the header info
      char * files_start_p = strstr(buf, "\r\n\r\n") + 4;

      long files_part_len = recv_len - (files_start_p - buf);

      ESP_LOGI(TAG, "OTA File Size: %d : Start Location:%d - End Location:%ld\r\n", remaining, *files_start_p, files_part_len);

      // Successful Upload: Flash first firmware chunk
      if (esp_ota_write(ota_handle, (const void *)files_start_p, files_part_len) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Flash Error");
        return ESP_FAIL;
      }
    } else {
      // Successful Upload: Flash firmware chunk
      if (esp_ota_write(ota_handle, (const void *)buf, recv_len) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Flash Error");
        return ESP_FAIL;
      }
    }

    remaining -= recv_len;
  }

  // Validate and switch to new OTA image and reboot
  if (esp_ota_end(ota_handle) != ESP_OK || esp_ota_set_boot_partition(ota_partition) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation / Activation Error");
    return ESP_FAIL;
  }

  char * response = success_response_json();
  httpd_resp_send(req, response, strlen(response));

  vTaskDelay(500 / portTICK_PERIOD_MS);
  esp_restart();

  return ESP_OK;
}

/* STATUS GET */
esp_err_t status_get_handler(httpd_req_t *req)
{
  /* Create session's context if not already available */
//  if (! req->sess_ctx) {
//    ESP_LOGI(TAG, "/status GET allocating new session");
//    req->sess_ctx = malloc(sizeof(int));
//    req->free_ctx = session_free_func;
//    *(int *)req->sess_ctx = 0;
//  }
//
//  ESP_LOGI(TAG, "/status GET handler send %d", *(int *)req->sess_ctx);

  /* Auth token validation */
//  if (validate_request(req) == ESP_OK)
//  {
    char * response = get_status_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);
//  } else {
//    httpd_resp_set_status(req, "401 Unauthorized!");
//    httpd_resp_send(req, NULL, 0);
//  }
  return ESP_OK;
}

/* SCHEDULE GET current data */
esp_err_t schedule_get_handler(httpd_req_t *req)
{
  /* Create session's context if not already available */
//  if (! req->sess_ctx) {
//    ESP_LOGI(TAG, "%s GET allocating new session", req->uri);
//    req->sess_ctx = malloc(sizeof(int));
//    req->free_ctx = session_free_func;
//    *(int *)req->sess_ctx = 0;
//  }
//  ESP_LOGI(TAG, "%s GET handler send %d", req->uri, *(int *)req->sess_ctx);

  /* Auth token validation */
  if (validate_request(req) == ESP_OK)
  {
    char * response = get_schedule_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);
  } else {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  }
  return ESP_OK;
}

/* SETTINGS GET current data */
esp_err_t settings_get_handler(httpd_req_t *req)
{
  /* Create session's context if not already available */
//  if (! req->sess_ctx) {
//    ESP_LOGI(TAG, "%s GET allocating new session", req->uri);
//    req->sess_ctx = malloc(sizeof(int));
//    req->free_ctx = session_free_func;
//    *(int *)req->sess_ctx = 0;
//  }
//  ESP_LOGI(TAG, "%s GET handler send %d", req->uri, *(int *)req->sess_ctx);

  /* Auth token validation */
  if (validate_request(req) == ESP_OK)
  {
    char * response = get_settings_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);
  } else {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  }
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

  /* Auth token validation */
  if (validate_request(req) != ESP_OK)
  {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  } else {
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

    /* channels duty --------------------------- */
    uint8_t channel_id = 0;
    double target_duty[MAX_LED_CHANNELS] = {0};
    double target_brightness = 0;
    cJSON *channel_item;
    cJSON *channels = cJSON_GetObjectItem(root, "channels");
    if (cJSON_IsArray(channels)) {
      cJSON_ArrayForEach(channel_item, channels) {
        if (cJSON_IsNumber(channel_item)) {
          target_duty[channel_id] = (double)channel_item->valueint;
          ESP_LOGI(TAG, "light_post_handler channel %d new duty %d", channel_id, channel_item->valueint);

        }
        channel_id++;
      }
    }

    /* channels brightness --------------------- */
    cJSON *brightness = cJSON_GetObjectItem(root, "brightness");
    if (cJSON_IsNumber(brightness)) {
      target_brightness = (double)brightness->valueint;
      ESP_LOGI(TAG, "light_post_handler new brightness %d", brightness->valueint);
    }

    set_light(target_duty, target_brightness, FAST, 0);

    cJSON_Delete(root);

    char *response = success_response_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);
  }
  free(buf);
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

  /* Auth token validation */
  if (validate_request(req) != ESP_OK)
  {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  } else {
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
      schedule_t *schedule_config = get_schedule(i);
      schedule_config->active = false;
    }

    /* channels schedule ----------------------- */
    uint8_t id = 0;
    cJSON *schedule_item;
    cJSON *schedule = cJSON_GetObjectItem(root, "schedule");
    cJSON_ArrayForEach(schedule_item, schedule) {
      schedule_t *schedule_config = get_schedule(id);

      cJSON *time_hour = cJSON_GetObjectItem(schedule_item, "time_hour");
      cJSON *time_minute = cJSON_GetObjectItem(schedule_item, "time_minute");
      cJSON *brightness = cJSON_GetObjectItem(schedule_item, "brightness");

      schedule_config->time_hour = time_hour->valueint;
      schedule_config->time_minute = time_minute->valueint;
      schedule_config->brightness = brightness->valueint;

      uint8_t channel_id = 0;
      cJSON *duty_item;
      cJSON *duty = cJSON_GetObjectItem(schedule_item, "duty");
      cJSON_ArrayForEach(duty_item, duty) {
        schedule_config->duty[channel_id] = duty_item->valueint;
        channel_id++;
      }
      schedule_config->active = true;
      id++;
    }

    cJSON_Delete(root);

    /* Store New Schedule in NVS */
    set_schedule();

    char *response = success_response_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);
  }
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
  /* Auth token validation */
  if (validate_request(req) != ESP_OK)
  {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  } else {
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
    cJSON *led_item;
    cJSON *led_channels = cJSON_GetObjectItem(root, "leds");
    if (cJSON_IsArray(led_channels)) {
      cJSON_ArrayForEach(led_item, led_channels) {
        cJSON *id = cJSON_GetObjectItem(led_item, "id");

        led_t *led_config = get_leds(id->valueint);

        cJSON *color = cJSON_GetObjectItem(led_item, "color");
        if (cJSON_IsString(color) && (color->valuestring != NULL)) {
          strlcpy(led_config->color, color->valuestring, 8);
        }

        cJSON *power = cJSON_GetObjectItem(led_item, "power");
        cJSON *duty_max = cJSON_GetObjectItem(led_item, "duty_max");
        cJSON *state = cJSON_GetObjectItem(led_item, "state");
        led_config->power = power->valueint;
        led_config->duty_max = duty_max->valueint;
        led_config->state = state->valueint;
      }

      /* Save Led NVS */
      set_led();
    }

    /* networks config ------------------------- */
    cJSON *network_item;
    cJSON *networks = cJSON_GetObjectItem(root, "networks");

    if (cJSON_IsArray(networks)) {
      if (cJSON_GetArraySize(networks) > 0) {

        /* Reset network cache */
        for (uint8_t i = 0; i < MAX_NETWORKS; i++) {
          network_t *network_config = get_networks(i);
          network_config->active = false;
        }

        uint8_t network_id = 0;
        cJSON_ArrayForEach(network_item, networks) {

          network_t *network_config = get_networks(network_id);

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
          network_id++;
        }
      } else {
        /* Reset network cache */
        for (uint8_t i = 0; i < MAX_NETWORKS; i++) {
          network_t *network_config = get_networks(i);
          network_config->active = false;
        }
      }

      /* Save Network NVS */
      set_network();
    }

    /* services config ------------------------- */
    cJSON *services = cJSON_GetObjectItem(root, "services");
    if (cJSON_IsObject(services)) {
      services_t *service_config = get_services();

      cJSON *hostname = cJSON_GetObjectItem(services, "hostname");
      if (cJSON_IsString(hostname) && (hostname->valuestring != NULL)) {
        strlcpy(service_config->hostname, hostname->valuestring, 32);
      }

      cJSON *ota_url = cJSON_GetObjectItem(services, "ota_url");
      if (cJSON_IsString(ota_url) && (ota_url->valuestring != NULL)) {
        strlcpy(service_config->ota_url, ota_url->valuestring, 64);
      }

      cJSON *ntp_server = cJSON_GetObjectItem(services, "ntp_server");
      if (cJSON_IsString(ntp_server) && (hostname->valuestring != NULL)) {
        strlcpy(service_config->ntp_server, ntp_server->valuestring, 32);
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

      /* Save Services NVS */
      set_service();

      /* reload services */

    }

    /* thingsboard config ------------------------- */
    cJSON *thingsboard = cJSON_GetObjectItem(root, "thingsboard");
    if (cJSON_IsObject(thingsboard)) {
      thingsboard_t *thingsboard_config = get_thingsboard();

      cJSON *token = cJSON_GetObjectItem(thingsboard, "token");
      if (cJSON_IsString(token) && (token->valuestring != NULL)) {
        strlcpy(thingsboard_config->token, token->valuestring, 32);
      }

      cJSON *endpoint = cJSON_GetObjectItem(thingsboard, "endpoint");
      if (cJSON_IsString(endpoint) && (endpoint->valuestring != NULL)) {
        strlcpy(thingsboard_config->endpoint, endpoint->valuestring, 64);
      }

      cJSON *qos = cJSON_GetObjectItem(thingsboard, "qos");
      thingsboard_config->qos = qos->valueint;

      cJSON *retain = cJSON_GetObjectItem(thingsboard, "retain");
      if (cJSON_IsTrue(retain)) {
        thingsboard_config->retain = true;
      } else {
        thingsboard_config->retain = false;
      }

      cJSON *enable = cJSON_GetObjectItem(thingsboard, "enable");
      if (cJSON_IsTrue(enable)) {
        thingsboard_config->enable = true;
      } else {
        thingsboard_config->enable = false;
      }

      cJSON *rpc = cJSON_GetObjectItem(thingsboard, "rpc");
      if (cJSON_IsTrue(rpc)) {
        thingsboard_config->rpc = true;
      } else {
        thingsboard_config->rpc = false;
      }

      /* Save ThingsBoard NVS */
      set_thingsboard();
    }

    /* time config ----------------------------- */
    cJSON *time = cJSON_GetObjectItem(root, "time");
    if (cJSON_IsObject(time)) {
      cJSON *year = cJSON_GetObjectItem(time, "year");
      cJSON *month = cJSON_GetObjectItem(time, "month");
      cJSON *day = cJSON_GetObjectItem(time, "day");
      cJSON *weekday = cJSON_GetObjectItem(time, "weekday");
      cJSON *hour = cJSON_GetObjectItem(time, "hour");
      cJSON *minute = cJSON_GetObjectItem(time, "minute");
      cJSON *second = cJSON_GetObjectItem(time, "second");

      datetime_t datetime;
      datetime.year = year->valueint;
      datetime.month = month->valueint;
      datetime.day = day->valueint;
      datetime.weekday = weekday->valueint;
      datetime.hour = hour->valueint;
      datetime.min = minute->valueint;
      datetime.sec = second->valueint;

      mcp7940_set_datetime(&datetime);

      char *debug_string = NULL;
      debug_string = cJSON_Print(time);
      if (debug_string == NULL) {
        fprintf(stderr, "Failed to print time.\n");
      } else {
        ESP_LOGI(TAG, "JSON parse time: %s", debug_string);
      }
      free(debug_string);
    }

    /* schedule config ------------------------- */
    cJSON *schedule_config_json = cJSON_GetObjectItem(root, "schedule_config");
    if (cJSON_IsObject(schedule_config_json)) {
      schedule_config_t *schedule_config = get_schedule_config();

      cJSON *mode = cJSON_GetObjectItem(schedule_config_json, "mode");
      schedule_config->mode = mode->valueint;

      cJSON *rgb = cJSON_GetObjectItem(schedule_config_json, "rgb");
      if (cJSON_IsTrue(rgb)) {
        schedule_config->rgb = true;
      } else {
        schedule_config->rgb = false;
      }

      cJSON *sunrise_hour = cJSON_GetObjectItem(schedule_config_json, "sunrise_hour");
      schedule_config->sunrise_hour = sunrise_hour->valueint;

      cJSON *sunrise_minute = cJSON_GetObjectItem(schedule_config_json, "sunrise_minute");
      schedule_config->sunrise_minute = sunrise_minute->valueint;

      cJSON *sunset_hour = cJSON_GetObjectItem(schedule_config_json, "sunset_hour");
      schedule_config->sunset_hour = sunset_hour->valueint;

      cJSON *sunset_minute = cJSON_GetObjectItem(schedule_config_json, "sunset_minute");
      schedule_config->sunset_minute = sunset_minute->valueint;

      cJSON *brightness = cJSON_GetObjectItem(schedule_config_json, "brightness");
      schedule_config->brightness = brightness->valueint;

      cJSON *gamma = cJSON_GetObjectItem(schedule_config_json, "gamma");
      schedule_config->gamma = gamma->valueint;

      uint8_t channel_id = 0;
      cJSON *duty_item;
      cJSON *duty = cJSON_GetObjectItem(schedule_config_json, "duty");
      cJSON_ArrayForEach(duty_item, duty) {
        schedule_config->duty[channel_id] = duty_item->valueint;
        channel_id++;
      }

      /* Save Schedule Config NVS */
      set_schedule_config();
    }

#ifdef USE_FAN_PWM
    /* cooling config ------------------------- */
    cJSON *cooling_json = cJSON_GetObjectItem(root, "cooling");
    if (cJSON_IsObject(cooling_json)) {
      cooling_t *cooling = get_cooling();

      cJSON *installed = cJSON_GetObjectItem(cooling_json, "installed");
      if (cJSON_IsTrue(installed)) {
        cooling->installed = true;
      } else {
        cooling->installed = false;
      }

      cJSON *start_temp = cJSON_GetObjectItem(cooling_json, "start_temp");
      cooling->start_temp = start_temp->valueint;

      cJSON *target_temp = cJSON_GetObjectItem(cooling_json, "target_temp");
      cooling->target_temp = target_temp->valueint;

      cJSON *max_temp = cJSON_GetObjectItem(cooling_json, "max_temp");
      cooling->max_temp = max_temp->valueint;

      cJSON *pid_kp = cJSON_GetObjectItem(cooling_json, "pid_kp");
      cooling->pid_kp = pid_kp->valuedouble;

      cJSON *pid_ki = cJSON_GetObjectItem(cooling_json, "pid_ki");
      cooling->pid_ki = pid_ki->valuedouble;

      cJSON *pid_kd = cJSON_GetObjectItem(cooling_json, "pid_kd");
      cooling->pid_kd = pid_kd->valuedouble;

      /* Save Cooling NVS */
      set_cooling();
    }
#endif

    /* auth ------------------------- */
    cJSON * user_json = cJSON_GetObjectItem(root, "auth");
    if (cJSON_IsObject(user_json)) {
      auth_t * auth = get_auth();

      cJSON * user = cJSON_GetObjectItem(user_json, "user");
      if (cJSON_IsString(user) && (user->valuestring != NULL)) {
        strlcpy(auth->user, user->valuestring, 32);
      }

      cJSON * password = cJSON_GetObjectItem(user_json, "password");
      if (cJSON_IsString(password) && (password->valuestring != NULL)) {
        strlcpy(auth->password, password->valuestring, 32);
      }

      /* Save Auth */
      set_auth();
    }

    cJSON_Delete(root);

    char *response = success_response_json();
    httpd_resp_send(req, response, strlen(response));
    free(response);
  }
  free(buf);
  return ESP_OK;
}

/* Auth handler */
static esp_err_t auth_post_handler(httpd_req_t *req)
{
  bool user_valid = false;
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

  cJSON * root = cJSON_Parse(buf);

  cJSON * user = cJSON_GetObjectItem(root, "user");
  cJSON * password = cJSON_GetObjectItem(root, "password");

  char * string = NULL;
  string = cJSON_Print(root);
  if (string == NULL)
  {
    printf("\n %s \n", string);
  }

  free(string);

  /* load saved user */
  auth_t * auth = get_auth();

  if (cJSON_IsString(user) && (user->valuestring != NULL)) {
    if(strncmp(user->valuestring, auth->user, strlen(auth->user)) == 0)
    {
      user_valid = true;
    }
  }

  if (cJSON_IsString(password) && (password->valuestring != NULL)) {
    if(strncmp(password->valuestring, auth->password, strlen(auth->password)) != 0 || !user_valid)
    {
      user_valid = false;
    }
  } else {
    user_valid = false;
  }

  cJSON_Delete(root);


  if (user_valid) {
    /* create new token */
    generateToken(auth_token);

    char * response = NULL;
    cJSON * json = cJSON_CreateObject();
    cJSON_AddTrueToObject(json, "success");
    cJSON_AddItemToObject(json, "token", cJSON_CreateString(auth_token));

    response = cJSON_Print(json);
    if (string == NULL)
    {
      fprintf(stderr, "Failed to print monitor.\n");
    }
    cJSON_Delete(json);

    httpd_resp_send(req, response, strlen(response));
    free(response);
  } else {
    httpd_resp_set_status(req, "401 Unauthorized!");
    httpd_resp_send(req, NULL, 0);
  }

  free(buf);
  return ESP_OK;
//  char*  buf;
//  size_t buf_len;
//  char* check;

//  buf_len = httpd_req_get_hdr_value_len(req, "Authorization") + 1;
//  if (buf_len > 1) {
//    buf = malloc(buf_len);
//    check=malloc(sizeof(WWW_USER_PASS_BASE64)+6);
//    if (httpd_req_get_hdr_value_str(req, "Authorization", buf, buf_len) == ESP_OK) {
//      sprintf(check,"Basic %s",WWW_USER_PASS_BASE64);
//      if(!strncmp(buf,check,strlen(check))){
//        ESP_LOGI(TAG, "%s", "OK");
//        httpd_resp_set_status(req, "200 Ok");
//        return 0;
//      }else{
//        ESP_LOGI(TAG, "%s", "NO");
//        httpd_resp_set_status(req, "403 Forbidden");
//      }
//    }
//    free(buf);
//    free(check);
//  }else{
//    httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=pippo");
//    ESP_LOGI(TAG, "WWW-Authoicate:%s","culo");
//    httpd_resp_set_status(req, "401 Unauthorized");
//    ESP_LOGI(TAG, "401:%s","culo");
//  }
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
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16;
  config.recv_wait_timeout = 30;
  config.send_wait_timeout = 60;
//  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

  // Start the httpd server
  if (server != NULL) {
    stop_webserver();
  }

  /* Auth token */
  generateToken(auth_token);

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

    /* Favicon */
    httpd_uri_t get_favicon = {
        .uri      = "/favicon.ico",
        .method   = HTTP_GET,
        .handler  = favicon_get_handler,
        .user_ctx = NULL
    };

    /* Reboot device */
    httpd_uri_t get_reboot = {
        .uri      = "/reboot",
        .method   = HTTP_GET,
        .handler  = reboot_get_handler,
        .user_ctx = NULL
    };

    /* Restore initial configuration */
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

    /* HTTP upload firmware */
    httpd_uri_t update_post = {
        .uri	  = "/upload",
        .method   = HTTP_POST,
        .handler  = upload_post_handler,
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

    /* POST Auth data */
    httpd_uri_t post_auth = {
        .uri      = "/auth",
        .method   = HTTP_POST,
        .handler  = auth_post_handler,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(server, &global_options);
    httpd_register_uri_handler(server, &get_reboot);
    httpd_register_uri_handler(server, &get_factory);
    httpd_register_uri_handler(server, &get_ota);
    httpd_register_uri_handler(server, &update_post);
    httpd_register_uri_handler(server, &get_home);
    httpd_register_uri_handler(server, &get_favicon);
    httpd_register_uri_handler(server, &get_status);
    httpd_register_uri_handler(server, &post_light);
    httpd_register_uri_handler(server, &get_schedule);
    httpd_register_uri_handler(server, &post_schedule);
    httpd_register_uri_handler(server, &get_settings);
    httpd_register_uri_handler(server, &post_settings);
    httpd_register_uri_handler(server, &post_auth);

    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

/* Stop the httpd server */
void stop_webserver(void)
{
  httpd_stop(server);
  server = NULL;
}

/* JSON -------------------------------------- */

/* http://device.name/status
    status: {
          upTime: '1 day 23:40',
          localTime: '12:00',
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
  system_status_t * system_status = get_system_status();
  services_t * services = get_services();

  cJSON * root = cJSON_CreateObject();
  cJSON * status = cJSON_CreateObject();

  det_time_string_since_boot((char*) &time_string);
  cJSON_AddItemToObject(status, "upTime", cJSON_CreateString(time_string));

  get_time_string((char*) &time_string);
  cJSON_AddItemToObject(status, "localTime", cJSON_CreateString(time_string));

  cJSON_AddItemToObject(status, "freeHeap", cJSON_CreateNumber(system_status->free_heap));
  cJSON_AddItemToObject(status, "vcc", cJSON_CreateNumber(read_vcc_voltage()));
  cJSON_AddItemToObject(status, "ntc_temperature", cJSON_CreateNumber(read_ntc_temperature()));
//  cJSON_AddItemToObject(status, "board_temperature", cJSON_CreateNumber(read_mcu_temperature()));
  cJSON_AddItemToObject(status, "wifiMode", cJSON_CreateString(system_status->wifi_mode));
  cJSON_AddItemToObject(status, "ipAddress", cJSON_CreateString(system_status->net_address));
  cJSON_AddItemToObject(status, "macAddress", cJSON_CreateString(system_status->mac));
  cJSON_AddItemToObject(status, "firmware", cJSON_CreateString(FIRMWARE));
  cJSON_AddItemToObject(status, "hardware", cJSON_CreateString(HARDWARE));

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
    schedule_t * schedule_config = get_schedule(i);
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

/* http://device.name/api/schedule_config
  schedule_config: {
    mode: 0,
    rgb: true,
    sunrise_hour: 10,
    sunrise_minute: 0,
    sunset_hour: 22,
    sunset_minute: 0,
    brightness: 100,
    duty: [
      10, 20, 10, 20, 20, 4, 10, 0
    ]
  }
*/
char * get_schedule_config_json()
{
  char *string = NULL;

  cJSON *root = cJSON_CreateObject();

  /* schedule config */
  cJSON * schedule_config_json = cJSON_CreateObject();
  schedule_config_t * schedule_config = get_schedule_config();
  cJSON_AddItemToObject(schedule_config_json, "mode", cJSON_CreateNumber(schedule_config->mode));
  cJSON_AddItemToObject(schedule_config_json, "rgb", cJSON_CreateBool(schedule_config->rgb));
  cJSON_AddItemToObject(schedule_config_json, "sunrise_hour", cJSON_CreateNumber(schedule_config->sunrise_hour));
  cJSON_AddItemToObject(schedule_config_json, "sunrise_minute", cJSON_CreateNumber(schedule_config->sunrise_minute));
  cJSON_AddItemToObject(schedule_config_json, "sunset_hour", cJSON_CreateNumber(schedule_config->sunset_hour));
  cJSON_AddItemToObject(schedule_config_json, "sunset_minute", cJSON_CreateNumber(schedule_config->sunset_minute));
  cJSON_AddItemToObject(schedule_config_json, "brightness", cJSON_CreateNumber(schedule_config->brightness));
  cJSON_AddItemToObject(schedule_config_json, "gamma", cJSON_CreateNumber(schedule_config->gamma));

  cJSON *schedule_config_duty = cJSON_CreateArray();
  for (int j = 0; j < MAX_LED_CHANNELS; ++j) {
    cJSON_AddItemToArray(schedule_config_duty, cJSON_CreateNumber(schedule_config->duty[j]));
  }
  cJSON_AddItemToObject(schedule_config_json, "duty", schedule_config_duty);
  cJSON_AddItemToObject(root, "schedule_config", schedule_config_json);

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
    }},
    schedule_config: {
      mode: 0,
      rgb: true,
      sunrise_hour: 10,
      sunrise_minute: 0,
      sunset_hour: 22,
      sunset_minute: 0,
      brightness: 100,
      duty: [
        10, 20, 10, 20, 20, 4, 10, 0
      ]
    }
*/
char * get_settings_json()
{
  char *string = NULL;

  cJSON * root = cJSON_CreateObject();

  /* led channels config */
  cJSON *led_channels = cJSON_CreateArray();
  for (int i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    led_t * led_config = get_leds(i);

    cJSON * led_item = cJSON_CreateObject();
    cJSON_AddItemToObject(led_item, "id", cJSON_CreateNumber(led_config->id));
    cJSON_AddItemToObject(led_item, "color", cJSON_CreateString(led_config->color));
    cJSON_AddItemToObject(led_item, "power", cJSON_CreateNumber(led_config->power));
    cJSON_AddItemToObject(led_item, "duty_max", cJSON_CreateNumber(led_config->duty_max));
    cJSON_AddItemToObject(led_item, "state", cJSON_CreateNumber(led_config->state));

    cJSON_AddItemToArray(led_channels, led_item);
  }
  cJSON_AddItemToObject(root, "leds", led_channels);

  /* networks config */
  cJSON * networks = cJSON_CreateArray();
  for (int j = 0; j < MAX_NETWORKS; ++j) {
    network_t * network_config = get_networks(j);
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
  services_t * service_config = get_services();
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

  /* thingsboard config */
  cJSON * thingsboard = cJSON_CreateObject();
  thingsboard_t * thingsboard_config = get_thingsboard();
  cJSON_AddItemToObject(thingsboard, "token", cJSON_CreateString(thingsboard_config->token));
  cJSON_AddItemToObject(thingsboard, "endpoint", cJSON_CreateString(thingsboard_config->endpoint));
  cJSON_AddItemToObject(thingsboard, "qos", cJSON_CreateNumber(thingsboard_config->qos));
  cJSON_AddItemToObject(thingsboard, "retain", cJSON_CreateBool(thingsboard_config->retain));
  cJSON_AddItemToObject(thingsboard, "enable", cJSON_CreateBool(thingsboard_config->enable));
  cJSON_AddItemToObject(thingsboard, "rpc", cJSON_CreateBool(thingsboard_config->rpc));

  cJSON_AddItemToObject(root, "thingsboard", thingsboard);

  /* time config */
  datetime_t datetime;
  mcp7940_get_datetime(&datetime);

  cJSON * time = cJSON_CreateObject();
  cJSON_AddItemToObject(time, "year", cJSON_CreateNumber(datetime.year));
  cJSON_AddItemToObject(time, "month", cJSON_CreateNumber(datetime.month));
  cJSON_AddItemToObject(time, "weekday", cJSON_CreateNumber(datetime.weekday));
  cJSON_AddItemToObject(time, "day", cJSON_CreateNumber(datetime.day));
  cJSON_AddItemToObject(time, "hour", cJSON_CreateNumber(datetime.hour));
  cJSON_AddItemToObject(time, "minute", cJSON_CreateNumber(datetime.min));
  cJSON_AddItemToObject(time, "second", cJSON_CreateNumber(datetime.sec));

  cJSON_AddItemToObject(root, "time", time);

  /* schedule config */
  cJSON * schedule_config_json = cJSON_CreateObject();
  schedule_config_t * schedule_config = get_schedule_config();
  cJSON_AddItemToObject(schedule_config_json, "mode", cJSON_CreateNumber(schedule_config->mode));
  cJSON_AddItemToObject(schedule_config_json, "rgb", cJSON_CreateBool(schedule_config->rgb));
  cJSON_AddItemToObject(schedule_config_json, "sunrise_hour", cJSON_CreateNumber(schedule_config->sunrise_hour));
  cJSON_AddItemToObject(schedule_config_json, "sunrise_minute", cJSON_CreateNumber(schedule_config->sunrise_minute));
  cJSON_AddItemToObject(schedule_config_json, "sunset_hour", cJSON_CreateNumber(schedule_config->sunset_hour));
  cJSON_AddItemToObject(schedule_config_json, "sunset_minute", cJSON_CreateNumber(schedule_config->sunset_minute));
  cJSON_AddItemToObject(schedule_config_json, "brightness", cJSON_CreateNumber(schedule_config->brightness));
  cJSON_AddItemToObject(schedule_config_json, "gamma", cJSON_CreateNumber(schedule_config->gamma));

  cJSON *schedule_config_duty = cJSON_CreateArray();
  for (int j = 0; j < MAX_LED_CHANNELS; ++j) {
    cJSON_AddItemToArray(schedule_config_duty, cJSON_CreateNumber(schedule_config->duty[j]));
  }
  cJSON_AddItemToObject(schedule_config_json, "duty", schedule_config_duty);
  cJSON_AddItemToObject(root, "schedule_config", schedule_config_json);

#ifdef USE_FAN_PWM
  /* cooling */
  cJSON * cooling_json = cJSON_CreateObject();
  cooling_t * cooling = get_cooling();
  cJSON_AddItemToObject(cooling_json, "installed", cJSON_CreateBool(cooling->installed));
  cJSON_AddItemToObject(cooling_json, "start_temp", cJSON_CreateNumber(cooling->start_temp));
  cJSON_AddItemToObject(cooling_json, "target_temp", cJSON_CreateNumber(cooling->target_temp));
  cJSON_AddItemToObject(cooling_json, "max_temp", cJSON_CreateNumber(cooling->max_temp));
  cJSON_AddItemToObject(cooling_json, "pid_kp", cJSON_CreateNumber(cooling->pid_kp));
  cJSON_AddItemToObject(cooling_json, "pid_ki", cJSON_CreateNumber(cooling->pid_ki));
  cJSON_AddItemToObject(cooling_json, "pid_kd", cJSON_CreateNumber(cooling->pid_kd));
  cJSON_AddItemToObject(cooling_json, "tachometer", cJSON_CreateNumber(get_fan_rpm()));
  cJSON_AddItemToObject(root, "cooling", cooling_json);
#endif

  /* auth */
  cJSON * user_json = cJSON_CreateObject();
  auth_t * auth = get_auth();
  cJSON_AddItemToObject(user_json, "user", cJSON_CreateString(auth->user));
  cJSON_AddItemToObject(user_json, "password", cJSON_CreateString(auth->password));
  cJSON_AddItemToObject(root, "auth", user_json);

  string = cJSON_Print(root);
  if (string == NULL)
  {
    fprintf(stderr, "Failed to print settings json.\n");
  }

  cJSON_Delete(root);
  return string;
}