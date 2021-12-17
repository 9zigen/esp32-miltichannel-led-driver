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
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include "adc.h"
#include <pwm.h>
#include <pidreg.h>
#include <fanspeed.h>
#include <auth.h>

#include "board.h"
#include "mcp7940.h"
#include "server.h"
#include "monitor.h"
#include "settings.h"
#include "rtc.h"
#include "mqtt.h"
#include "light.h"
#include "main.h"
#include "buttons.h"
#include "thingsboard.h"
#include "connect.h"
#include "led.h"
#include "udp_multicast.h"
#include "pwm.h"
#include "ota.h"

static const char *TAG="APP";
static uint8_t ota_requested = 0;

esp_err_t upgrade_firmware(void)
{
  /* disable wifi powersave */
//  disable_power_save();

  services_t * services = get_services();
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
    set_led_mode(LED_SLOW_BLINK);
    return ESP_OK;
  } else {
    set_led_mode( LED_THREE_BLINK);
    return ESP_ERR_NO_MEM;
  }
}

void app_main()
{
  /* NVS Init */
  init_settings();

  /* Force Init OTA */
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
  vTaskDelay(1000 / portTICK_RATE_MS);
  if (!gpio_get_level(GPIO_NUM_0))
  {
//    upgrade_firmware();
//    vTaskDelay(120 * 60 * 1000 / portTICK_RATE_MS);
    erase_settings();
    set_led_mode(LED_FAST_BLINK);
    vTaskDelay(5000 / portTICK_RATE_MS);
    esp_restart();
  }

  /* Notification LED Task */
  xTaskCreate(&task_led, "task_led", 768, NULL, 4, NULL);

  /* LEDC */
  init_ledc();

  /* WiFi + Web server */
  initialise_wifi(NULL);

  /* Buttons */
  init_buttons();

#ifdef USE_RTC
  /* MCP7940 */
  mcp7940_init();
#endif

  /* RTC Setup */
  init_clock();

  /* ADC1 */
  init_adc();

  /* Monitor Task */
  ESP_LOGI(TAG, "[APP] free memory before start monitor task %d bytes", esp_get_free_heap_size());
  xTaskCreate(&task_monitor, "task_monitor", 2048, NULL, 5, NULL);

  /* Light Task */
  ESP_LOGI(TAG, "[APP] free memory before start light task %d bytes", esp_get_free_heap_size());
  xTaskCreate(&task_light, "task_light", 2048, NULL, 5, NULL);

  /* Light Transition Task */
  ESP_LOGI(TAG, "[APP] free memory before start light transition task %d bytes", esp_get_free_heap_size());
  xTaskCreate(&task_light_transition, "task_light_transition", 2048, NULL, 5, NULL);

#ifdef USE_FAN_PWM
  /* Fan PID Task */
  ESP_LOGI(TAG, "[APP] free memory before fan pid task %d bytes", esp_get_free_heap_size());
  //fan_speed_init();
  xTaskCreate(&task_pid_calc, "task_pid_calc", 2048, NULL, 3, NULL);
#endif

  /* web server */
  ESP_LOGI(TAG, "[APP] free memory before start web server %d bytes", esp_get_free_heap_size());
  start_webserver();

  /* Wait WiFi */
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

  /* UDP Sync Task */
  xTaskCreate(&mcast_udp_task, "mcast_udp_task", 4096, NULL, 5, NULL);

  /* MQTT Task */
  xTaskCreate(&task_mqtt, "mqtt_task", 2048, NULL, 5, NULL);

  /* ThingsBoard Task */
#ifdef THINGSBOARD
  xTaskCreate(&task_thingsboard, "thingsboard_task", 2048, NULL, 5, NULL);
#endif
}
