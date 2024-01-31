/***
** Created by Aleksey Volkov on 16.12.2019.
***/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/xtensa_rtos.h"

#include "time.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

#include "board.h"
#include "tools.h"
#include "pwm.h"
#include "adc.h"
#include "rtc.h"
#include "string.h"
#include "light.h"
#include "fanspeed.h"
#include "monitor.h"

static const char *TAG="MONITOR";
system_status_t system_status;
TimerHandle_t xMonitorTimer;

static void update_system_info()
{
  /* Wi-Fi info */
  wifi_mode_t mode;
  ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
  if (mode == WIFI_MODE_STA)
    snprintf(system_status.wifi_mode, 6, "STA");
  else if (mode == WIFI_MODE_AP)
    snprintf(system_status.wifi_mode, 6, "AP");
  else if (mode == WIFI_MODE_APSTA)
    snprintf(system_status.wifi_mode, 6, "APSTA");

  /* MAC */
  uint8_t mac[6];

  /* IP info */
  esp_netif_ip_info_t ip;
  esp_netif_dns_info_t dns;
  memset(&ip, 0, sizeof(esp_netif_ip_info_t));
  memset(&dns, 0, sizeof(esp_netif_dns_info_t));

  if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip) == ESP_OK)
  {
    snprintf(system_status.net_address, 16, IPSTR, IP2STR(&ip.ip));
    snprintf(system_status.net_mask, 16, IPSTR, IP2STR(&ip.netmask));
    snprintf(system_status.net_gateway, 16, IPSTR, IP2STR(&ip.gw));
    snprintf(system_status.net_gateway, 16, IPSTR, IP2STR(&ip.gw));
    esp_wifi_get_mac(WIFI_IF_STA, (uint8_t*)&mac);
    snprintf(system_status.mac, 18, MACSTR, MAC2STR(mac));
  }
  else if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip) == ESP_OK)
  {
    snprintf(system_status.net_address, 16, IPSTR, IP2STR(&ip.ip));
    snprintf(system_status.net_mask, 16, IPSTR, IP2STR(&ip.netmask));
    snprintf(system_status.net_gateway, 16, IPSTR, IP2STR(&ip.gw));
    esp_wifi_get_mac(WIFI_IF_AP, (uint8_t*)&mac);
    snprintf(system_status.mac, 18, MACSTR, MAC2STR(mac));
  }

  ESP_LOGD(TAG, "net address: %s", system_status.net_address);
  ESP_LOGD(TAG, "net mac    : %s", system_status.mac);

  /* Update time from RTC */
//  set_time_from_stm();

  /* Mcu temp sensor */
//  system_status.mcu_temp = (double) read_mcu_temperature() / 100.0;
#ifdef PICO_D4_5CH_LED_DRIVER_AIO
  system_status.mcu_temp = 40;
#endif
  /* Local time */
//  print_time();

  /* Free heap */
  system_status.free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);

  /* ADC: NTC + VCC */
  ESP_LOGD(TAG, "ntc temp   : %d", read_ntc_temperature());
  ESP_LOGD(TAG, "mcu temp   : %.2f", system_status.mcu_temp);
  ESP_LOGD(TAG, "power in   : %.2f", read_vcc_voltage());
  ESP_LOGI(TAG, "free heap  : %u", heap_caps_get_free_size(MALLOC_CAP_8BIT));

#ifdef USE_FAN_PWM
  ESP_LOGD(TAG, "fan pwm    : %lu", ledc_fan_get());
  ESP_LOGD(TAG, "fan spd    : %lu", get_fan_rpm());
#endif

  /* overheat protection
     * ToDo web ui configuration for step and max temperature */
//  if (read_ntc_temperature() > 65 && read_mcu_temperature() > 6500) {
//    uint8_t down_brightness = get_brightness() - 2;
//    set_brightness(down_brightness);
//    ESP_LOGE(TAG, "ntc sensor temperature too high, down brightness to %d", down_brightness);
//  }
}

system_status_t* get_system_status(void)
{
  return &system_status;
}

static void vMonitorTimerCallback(TimerHandle_t xTimer )
{
  update_system_info();
}

int init_monitor()
{
  update_system_info();
  /* Create pump auto stop timer with 100ms. period */
  xMonitorTimer = xTimerCreate("xRunTimer", 30 * 1000 / portTICK_PERIOD_MS, pdTRUE, NULL, vMonitorTimerCallback);
  CHECK_TIMER(xTimerStart(xMonitorTimer, 100 / portTICK_PERIOD_MS));

  return ESP_OK;
}