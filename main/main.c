/* Persistent Sockets Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <server.h>
#include <monitor.h>
#include <stmdriver.h>
#include <settings.h>
#include <rtc.h>
#include <mqtt.h>
#include <light.h>
#include <main.h>
#include <buttons.h>

#include "led.h"
#include "ota.h"

static const char *TAG="APP";

/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group;
TimerHandle_t xTimerSTA;
const int CONNECTED_BIT = BIT0;
const int AP_CONNECTED_BIT = BIT1;
uint16_t disconnect_count = 0;
uint16_t reconnect_period = 30;

/* Private prototipes */
static void wifi_init_sta(network_t * network_config);
static void wifi_init_ap();
static esp_err_t connect_wifi(uint8_t was_fail);

static httpd_handle_t server = NULL;
static uint8_t ota_requested = 0;

static void vTimerCallback( TimerHandle_t xTimer )
{
  xTimerStop( xTimer, 0 );

  /* Clear STA Disconnects */
  disconnect_count = 0;

  /* Start STA */
  /* check saved network config else start AP */
  if (connect_wifi(1) != ESP_OK)
  {
    ESP_ERROR_CHECK(esp_wifi_stop());
    wifi_init_ap(0);
  }

  ESP_LOGI(TAG, "vTimerCallback");
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  httpd_handle_t *server = (httpd_handle_t *) ctx;
  /* For accessing reason codes in case of disconnection */
  system_event_info_t *info = &event->event_info;

  switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
      ESP_ERROR_CHECK(esp_wifi_connect());
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
      ESP_LOGI(TAG, "Got IP: '%s'", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
      /* Start the web server */
      if (!ota_requested && *server == NULL) {
        *server = start_webserver();
      }
      /* disable AP if enabled */
      ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
      /* Stop reconnect timer if started */
      if (xTimerIsTimerActive(xTimerSTA)) {
        xTimerStop(xTimerSTA, 100);
      }
      /* Change Led mode & set wifi_event_group flag */
      set_led_mode(LED_SLOW_BLINK);
      xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
      ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
      if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
        /*Switch to 802.11 bgn mode */
        esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
      }
      ESP_ERROR_CHECK(esp_wifi_connect());
      /* Stop the webserver */
      if (*server) {
        stop_webserver(*server);
      }
      disconnect_count ++;
      if (disconnect_count == 5)
      {
        ESP_LOGI(TAG, "STA Disconnect: %d. Starting AP...", disconnect_count);
        ESP_ERROR_CHECK(esp_wifi_stop());
        wifi_init_ap();

        /* start STA auto reconnect timer */
        if( xTimerStart( xTimerSTA, 100 ) != pdPASS ) {
          ESP_LOGI(TAG, "error start STA auto reconnect time");
        }
      }
      /* Change Led mode & set wifi_event_group flag */
      set_led_mode(LED_THREE_BLINK);
      xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
      break;

    case SYSTEM_EVENT_AP_STACONNECTED:
      ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED");
      /* Start the web server */
      if (!ota_requested && *server == NULL) {
        *server = start_webserver();
      }
      /* Stop reconnect timer if started */
      if (xTimerIsTimerActive(xTimerSTA)) {
        xTimerStop(xTimerSTA, 100);
      }
      /* Change Led mode & set wifi_event_group flag */
      set_led_mode(LED_SLOW_BLINK);
      xEventGroupSetBits(wifi_event_group, AP_CONNECTED_BIT);
      break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
      ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED");
      /* Stop the webserver */
      if (*server) {
        stop_webserver(*server);
      }
      /* start STA auto reconnect timer */
      if( xTimerStart( xTimerSTA, 100 ) != pdPASS ) {
        ESP_LOGI(TAG, "error start STA auto reconnect time");
      }
      /* Change Led mode & set wifi_event_group flag */
      set_led_mode(LED_TWO_BLINK);
      xEventGroupClearBits(wifi_event_group, AP_CONNECTED_BIT);
      break;
    default:
      break;
  }
  /* Forward event to to the WiFi CGI module */
  //cgiWifiEventCb(event);

  return ESP_OK;
}

static void wifi_init_ap()
{

  wifi_config_t wifi_config = {
      .ap = {
          .ssid = CONFIG_CONTROLLER_WIFI_SSID,
          .ssid_len = strlen(CONFIG_CONTROLLER_WIFI_SSID),
          .password = CONFIG_CONTROLLER_WIFI_PASS,
          .max_connection = 1,
          .authmode = WIFI_AUTH_WPA_WPA2_PSK
      },
  };
  if (strlen(CONFIG_CONTROLLER_WIFI_PASS) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
           CONFIG_CONTROLLER_WIFI_SSID, CONFIG_CONTROLLER_WIFI_PASS);

  set_led_mode(LED_FAST_BLINK);
}

static void wifi_init_sta(network_t * network_config)
{
  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", network_config->ssid);
  wifi_config_t wifi_config;
  strlcpy((char*)&wifi_config.sta.ssid, network_config->ssid, 32);
  strlcpy((char*)&wifi_config.sta.password, network_config->password, 64);
  ESP_LOGI(TAG, "SSID    :%s", wifi_config.sta.ssid);
  ESP_LOGI(TAG, "PASSWORD:%s", wifi_config.sta.password);

  wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
  wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
  wifi_config.sta.threshold.rssi = -127;
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  wifi_config.sta.listen_interval = 3; // this is the main setting, 3 is lowest

  /* Manual IP */
  if (!network_config->dhcp)
  {
    ESP_LOGI(TAG, "Setting adapter manual ip");

    char buff[16];
    tcpip_adapter_ip_info_t ip_info;
    uint8_t error = 0;

    /* IP */
    ip_to_string(network_config->ip_address, buff);
    if (!ip4addr_aton(buff, &ip_info.ip))
    {
      ESP_LOGE(TAG, "error ip     :%s", ip4addr_ntoa(&ip_info.ip));
      error = 1;
    }

    /* NetMask*/
    ip_to_string(network_config->mask, buff);
    if (!ip4addr_aton(buff, &ip_info.netmask))
    {
      ESP_LOGE(TAG, "error netmask:%s", ip4addr_ntoa(&ip_info.netmask));
      error = 1;
    }

    /* Gateway */
    ip_to_string(network_config->gateway, buff);
    if (!ip4addr_aton(buff, &ip_info.gw))
    {
      ESP_LOGE(TAG, "error gw     :%s", ip4addr_ntoa(&ip_info.gw));
      error = 1;
    }

    /* Fail to set manual ip, start AP */
    if (error)
    {
      wifi_init_ap();
      return;
    }

    ESP_LOGI(TAG, "set ip     :%s", ip4addr_ntoa(&ip_info.ip));
    ESP_LOGI(TAG, "set netmask:%s", ip4addr_ntoa(&ip_info.netmask));
    ESP_LOGI(TAG, "set gw     :%s", ip4addr_ntoa(&ip_info.gw));
  } else {
    ESP_LOGI(TAG, "Setting adapter DHCP");
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM));


  ESP_LOGI(TAG, "wifi_init_sta finished.");
  ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
           network_config->ssid, network_config->password);
}

static esp_err_t connect_wifi(uint8_t was_fail)
{
  static uint8_t active_index = 0;
  uint8_t total_networks = 0;

  /* check saved network config else start AP */
  for (int i = 0; i < MAX_NETWORKS; ++i) {
    network_t * network_cfg = get_network_config(i);
    if (network_cfg->active)
    {
      total_networks++;
    }
  }

  /* try next SSID */
  if (was_fail && total_networks > 1) {
    active_index++;
    if (active_index == MAX_NETWORKS) {
      active_index = 0;
    }
  }

  if (was_fail && total_networks) {
    active_index = 0;

    ESP_ERROR_CHECK(esp_wifi_stop());

    network_t * network_cfg = get_network_config(active_index);
    wifi_init_sta(network_cfg);
    return ESP_OK;
  } else if (total_networks) {
    active_index = 0;

    network_t * network_cfg = get_network_config(active_index);
    wifi_init_sta(network_cfg);
    return ESP_OK;
  } else {
    return ESP_FAIL;
  }
}

static void initialise_wifi(void *arg)
{
  services_t * services = get_service_config();

  tcpip_adapter_init();
  tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, services->hostname);

  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, arg));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  /* check saved network config else start AP */
  if (connect_wifi(0) != ESP_OK)
  {
    ESP_LOGI(TAG, "No Active WiFi Config. Starting AP");
    wifi_init_ap();
  }
}

esp_err_t upgrade_firmware()
{
  services_t * services = get_service_config();
  char * ota_url_ptr = NULL;
  if (strlen(services->ota_url) > 16)
  {
    ota_url_ptr = services->ota_url;
    ESP_LOGI(TAG, "update firmware from %s", ota_url_ptr);
  }

  /* Set OTA flag to prevent starting web server in event loop */
  ota_requested = 1;

  /* start ota task */
  if (xTaskCreate(&ota_task, "ota_task", 8192, ota_url_ptr, 5, NULL) != pdPASS)
  {
    return ESP_OK;
  } else {
    return ESP_ERR_NO_MEM;
  }
}

void app_main()
{
  /* NVS Init */
  init_settings();

  /* Notification LED Task */
  xTaskCreate(&task_led, "task_led", 2048, NULL, 5, NULL);

  /* Create STA auto reconnect timer with 30 sec. period */
  xTimerSTA = xTimerCreate("TimerSTA",30 * 1000/portTICK_RATE_MS, pdTRUE, NULL, vTimerCallback);

  /* WiFi + Web server */
  initialise_wifi(&server);

  /* Force Init OTA */
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
  vTaskDelay(3000 / portTICK_RATE_MS);
  if (!gpio_get_level(GPIO_NUM_0))
  {
    upgrade_firmware();
    return;
  }

  /* Buttons */
  init_buttons();

  /* STM Setup */
  init_stm();

  /* RTC Setup */
  init_clock();

  /* MQTT Setup */
  init_mqtt();

  /* Monitor Task */
  ESP_LOGI(TAG, "[APP] free memory before start monitor task %d bytes", esp_get_free_heap_size());
  xTaskCreate(&task_monitor, "task_monitor", 2048, NULL, 5, NULL);

  /* Light Task */
  ESP_LOGI(TAG, "[APP] free memory before start light task %d bytes", esp_get_free_heap_size());
  xTaskCreate(&task_light, "task_light", 4096, NULL, 5, NULL);

  /* Light Transition Task */
  ESP_LOGI(TAG, "[APP] free memory before start light transition task %d bytes", esp_get_free_heap_size());
  xTaskCreate(&task_light_transition, "task_light_transition", 4096, NULL, 5, NULL);

}
