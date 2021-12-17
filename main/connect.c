/***
** Created by Aleksey Volkov on 16.07.2020.
***/
#include <led.h>
#include "string.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mdns.h"

#include "connect.h"
#include "settings.h"

#define AP_WIFI_SSID          CONFIG_CONTROLLER_WIFI_SSID
#define AP_WIFI_PASSWORD      CONFIG_CONTROLLER_WIFI_PASS
#define AP_WIFI_CHANNEL       CONFIG_CONTROLLER_WIFI_CHANNEL
#define WIFI_MAXIMUM_RETRY    5
#define STA_WIFI_DISABLED     1

static void initialise_mdns(void);

static const char *TAG="CONNECT";
esp_netif_t* wifi_netif;
esp_event_handler_instance_t instance_any_id;
esp_event_handler_instance_t instance_got_ip;
static int s_retry_num = 0;
TimerHandle_t xReconnectTimer = NULL;
uint8_t total_sta_point = 0;
uint8_t active_sta_point = 0;

/* Static functions */
static esp_err_t wifi_reinit_sta(network_t * config);
static esp_err_t wifi_init_sta(network_t * config);
static esp_err_t wifi_init_sta_after_ap(network_t * config);
static void wifi_stop(void);
static void wifi_init_softap(void);

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();

  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < WIFI_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
      set_led_mode(LED_TWO_BLINK);
    } else if (total_sta_point > 1 && active_sta_point < MAX_NETWORKS) {
      network_t * network_config = get_networks(active_sta_point + 1);

      if (network_config->active && strlen(network_config->ssid) > 2)
      {
        active_sta_point++;
        s_retry_num = 0;
        wifi_reinit_sta(network_config);
        esp_wifi_connect();
      }
    } else {
      xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
      if( xReconnectTimer != NULL ) {
        ESP_LOGI(TAG,"connect to the AP fail, reconnect timer started");
        xTimerStart( xReconnectTimer, 100 / portTICK_RATE_MS);
        set_led_mode(LED_THREE_BLINK);
      }
    }
    ESP_LOGI(TAG,"connect to the AP fail");

  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    set_led_mode(LED_FAST_BLINK);

  } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
             MAC2STR(event->mac), event->aid);

  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d",
             MAC2STR(event->mac), event->aid);

  }
}

/* reset retry counter and try to connect tu AP */
void vReconnectTimerCallback( TimerHandle_t pxTimer )
{
  /* reconnect to AP */
  s_retry_num = 0;

  wifi_mode_t mode = WIFI_MODE_NULL;
  esp_wifi_get_mode(&mode);

  network_t * network_config = get_networks(active_sta_point);

  if (network_config->active &&  mode == WIFI_MODE_AP) {
    wifi_init_sta_after_ap(network_config);
  }
  esp_wifi_connect();
  ESP_LOGI(TAG, "reconnect to AP");
}

static void wifi_stop(void)
{
  /* The event will not be processed after unregister */
//  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
//  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));

  esp_wifi_disconnect();

  esp_err_t err = esp_wifi_stop();
  if (err == ESP_ERR_WIFI_NOT_INIT) {
    return;
  }
  ESP_ERROR_CHECK(err);
  ESP_ERROR_CHECK(esp_wifi_deinit());
  ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
  esp_netif_destroy(wifi_netif);
  wifi_netif = NULL;
}

static esp_err_t wifi_reinit_sta(network_t * config)
{
  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", config->ssid);
  wifi_config_t wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;
  strlcpy((char*)&wifi_config.sta.ssid, config->ssid, 32);
  strlcpy((char*)&wifi_config.sta.password, config->password, 64);
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_LOGI(TAG, "SSID    :%s", wifi_config.sta.ssid);
  ESP_LOGI(TAG, "PASSWORD:%s", wifi_config.sta.password);

  return esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
}

static esp_err_t wifi_init_sta_after_ap(network_t * config)
{
  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", config->ssid);
  wifi_config_t wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;
  strlcpy((char*)&wifi_config.sta.ssid, config->ssid, 32);
  strlcpy((char*)&wifi_config.sta.password, config->password, 64);
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_LOGI(TAG, "SSID    :%s", wifi_config.sta.ssid);
  ESP_LOGI(TAG, "PASSWORD:%s", wifi_config.sta.password);

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta_after_ap finished.");

  return ESP_OK;
}

static esp_err_t wifi_init_sta(network_t * config)
{
  if (wifi_netif != NULL)
  {
    wifi_stop();
  }

  ESP_ERROR_CHECK(esp_netif_init());
  wifi_netif = esp_netif_create_default_wifi_sta();

  services_t * services = get_services();
  esp_netif_set_hostname(wifi_netif, services->hostname);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_any_id));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_got_ip));

  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", config->ssid);
  wifi_config_t wifi_config;
  memset(&wifi_config, 0, sizeof(wifi_config));
  wifi_config.sta.pmf_cfg.capable = true;
  wifi_config.sta.pmf_cfg.required = false;
  strlcpy((char*)&wifi_config.sta.ssid, config->ssid, 32);
  strlcpy((char*)&wifi_config.sta.password, config->password, 64);
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_LOGI(TAG, "SSID    :%s", wifi_config.sta.ssid);
  ESP_LOGI(TAG, "PASSWORD:%s", wifi_config.sta.password);

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE) );
  ESP_ERROR_CHECK(esp_wifi_start() );

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
   * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
   * happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    network_t * active_wifi_config = get_networks(active_sta_point);
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", active_wifi_config->ssid, active_wifi_config->password);
    return ESP_OK;

  } else if (bits & WIFI_FAIL_BIT) {
    network_t * active_wifi_config = get_networks(active_sta_point);
    ESP_LOGI(TAG, "Failed to ap SSID:%s password:%s", active_wifi_config->ssid, active_wifi_config->password);
    return ESP_ERR_WIFI_MODE;

  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
    return ESP_ERR_WIFI_MODE;
  }
}

static void wifi_init_softap(void)
{
  ESP_ERROR_CHECK(esp_netif_init());
  wifi_netif = esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL));

  wifi_config_t wifi_config = {
      .ap = {
          .ssid = AP_WIFI_SSID,
          .ssid_len = strlen(AP_WIFI_SSID),
          .channel = AP_WIFI_CHANNEL,
          .password = AP_WIFI_PASSWORD,
          .max_connection = 2,
          .authmode = WIFI_AUTH_WPA2_PSK
      },
  };

  services_t * services = get_services();
  uint8_t ssid_len = strlen(services->hostname);
  if (ssid_len && ssid_len < 32)
  {
    for (uint8_t i = 0; i < ssid_len; ++i) {
      wifi_config.ap.ssid[i] = services->hostname[i];
    }
    wifi_config.ap.ssid_len = ssid_len;
  }

  if (strlen(AP_WIFI_PASSWORD) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
           AP_WIFI_SSID, AP_WIFI_PASSWORD, AP_WIFI_CHANNEL);
}

static esp_err_t connect_wifi(void)
{
  total_sta_point = 0;

  for (uint8_t i = 0; i < MAX_NETWORKS; ++i) {
    network_t * network_config = get_networks(i);

    if (network_config->active && strlen(network_config->ssid) > 2)
    {
      total_sta_point++;
    }
  }

  if (!total_sta_point) {
    return STA_WIFI_DISABLED;
  }

  for (uint8_t i = 0; i < MAX_NETWORKS; ++i) {
    network_t * network_config = get_networks(i);

    if (network_config->active && strlen(network_config->ssid) > 2)
    {
      if (strncmp(network_config->ssid, " ", 1) == 0) {
        return ESP_ERR_WIFI_MODE;
      }
      active_sta_point = i;
      return wifi_init_sta(network_config);
    }
  }

  return ESP_ERR_WIFI_MODE;
}

void initialise_wifi(void *arg)
{
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  /* reconnect timer */
  xReconnectTimer = xTimerCreate( "WiFiStaTimer", ( 300 * 1000 / portTICK_PERIOD_MS), pdFALSE, 0, vReconnectTimerCallback);

  initialise_mdns();

  /* check saved network config else start AP */
  if (connect_wifi() != ESP_OK)
  {
    ESP_LOGI(TAG, "STA Failed or Disabled");
    if (wifi_netif != NULL)
    {
      wifi_stop();
    }
    wifi_init_softap();
  }
}

void disable_power_save(void)
{
//  esp_wifi_set_ps(WIFI_PS_NONE);
}

/* initialize mDNS */
static void initialise_mdns(void)
{
  services_t * services = get_services();

  ESP_ERROR_CHECK( mdns_init() );
  ESP_ERROR_CHECK( mdns_hostname_set(services->hostname) );
  ESP_LOGI(TAG, "mdns hostname set to: [%s]", services->hostname);

  /* set default mDNS instance name */
  ESP_ERROR_CHECK( mdns_instance_name_set("ESP32 Led Controller") );

  mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

  /* initialize service */
//  ESP_ERROR_CHECK( mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData, 3) );
  //add another TXT item
//  ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "path", "/") );

  //structure with TXT records
//  mdns_txt_item_t serviceTxtData[3] = {
//      {"board","esp32"},
//      {"model","doser"}
//  };
}