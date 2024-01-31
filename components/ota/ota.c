/***
** Created by Aleksey Volkov on 22.12.2019.
***/

#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_tls.h"

#include "include/ota.h"
#include "../../main/include/connect.h"

static const char *TAG="OTA";

const char * default_ota_url_ptr = CONFIG_OTA_URL;
extern const uint8_t server_cert_pem_start[] asm("_binary_fw_alab_cc_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_fw_alab_cc_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      break;
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
      break;
    case HTTP_EVENT_REDIRECT:
      ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
      break;
  }
  return ESP_OK;
}

void ota_task(void *ota_url)
{
  ESP_LOGI(TAG, "Starting OTA ...");

  /* Wait for the callback to set the AP_CONNECTED_BIT in the
     event group.
  */
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                      false, true, portMAX_DELAY);
  ESP_LOGI(TAG, "Connect to Wifi ! Start to Connect to Server....");

  /* Custom CA bundle */
//  esp_tls_cfg_t cfg = {
//      .crt_bundle_attach = esp_crt_bundle_attach,
//  };
//  esp_crt_bundle_attach(NULL);
//  esp_tls_init_global_ca_store();

  esp_http_client_config_t config = {
      .url = default_ota_url_ptr,
      .timeout_ms = 20000,
      .keep_alive_enable = true,
      .event_handler = _http_event_handler,
      //      .cert_pem = (char *)server_cert_pem_start,
      //      .use_global_ca_store = true
  };

  /* ex: http://192.168.1.2:80/firmware.bin */
  if (strlen(ota_url)) {
    config.url = ota_url;
  }

  esp_https_ota_config_t ota_config = {
      .http_config = &config,
  };

  esp_err_t ret = esp_https_ota(&ota_config);

  if (ret == ESP_OK) {
    esp_restart();
  } else {
    ESP_LOGE(TAG, "Firmware Upgrades Failed");
  }

  /* exit OTA */
  ESP_LOGI(TAG, "Firmware Upgrades EXIT!");

  free(ota_url);
  vTaskDelete(NULL);
}