/***
** Created by Aleksey Volkov on 16.12.2019.
***/

#include <stmdriver.h>
#include <rtc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/xtensa_rtos.h"

#include "string.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "monitor.h"

static const char *TAG="MONITOR";
system_status_t system_status = {0};

static void update_system_info()
{
  /* WiFi info */
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
  tcpip_adapter_ip_info_t ip;
  tcpip_adapter_dns_info_t dns;
  memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
  memset(&dns, 0, sizeof(tcpip_adapter_dns_info_t));

  if (tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip) == ESP_OK)
  {
    snprintf(system_status.net_address, 16, IPSTR, IP2STR(&ip.ip));
    snprintf(system_status.net_mask, 16, IPSTR, IP2STR(&ip.netmask));
    snprintf(system_status.net_gateway, 16, IPSTR, IP2STR(&ip.gw));
    snprintf(system_status.net_gateway, 16, IPSTR, IP2STR(&ip.gw));
    esp_wifi_get_mac(ESP_IF_WIFI_STA, (uint8_t*)&mac);
    snprintf(system_status.mac, 18, MACSTR, MAC2STR(mac));
  }
  else if (tcpip_adapter_get_ip_info(ESP_IF_WIFI_AP, &ip) == ESP_OK)
  {
    snprintf(system_status.net_address, 16, IPSTR, IP2STR(&ip.ip));
    snprintf(system_status.net_mask, 16, IPSTR, IP2STR(&ip.netmask));
    snprintf(system_status.net_gateway, 16, IPSTR, IP2STR(&ip.gw));
    esp_wifi_get_mac(ESP_IF_WIFI_AP, (uint8_t*)&mac);
    snprintf(system_status.mac, 18, MACSTR, MAC2STR(mac));
  }

  ESP_LOGI(TAG, "net address: %s", system_status.net_address);
  ESP_LOGI(TAG, "net mask   : %s", system_status.net_mask);
  ESP_LOGI(TAG, "net gateway: %s", system_status.net_gateway);
  ESP_LOGI(TAG, "net mac    : %s", system_status.mac);

  /* Free heap */
  system_status.free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  ESP_LOGI(TAG, "free heap  : %u", system_status.free_heap);

  /* Local time */
  print_time();

  /* ToDo Led PWM */

  /* ToDo Led Brightness */


}

system_status_t* get_system_status(void)
{
  return &system_status;
}

void task_monitor(void *pvParameters)
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);


  while (1) {
    /* update wifi, network address, mem status */
    update_system_info();

    //xTaskCreate(&task_i2cscanner, "i2cScanner", 1024, NULL, 5, NULL);
    vTaskDelay(1000 * 60/portTICK_RATE_MS);
  }
}