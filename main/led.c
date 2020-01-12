/***
** Created by Aleksey Volkov on 07.01.2020.
***/
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include <esp_system.h>
#include <driver/gpio.h>

#include "include/led.h"

/* LED Notification */
uint8_t modes[] = {
    0B00000000, /* Off */
    0B11111111, /* On */
    0B00001111, /* Half second blinking */
    0B00000001, /* Short flash once per second */
    0B00000101, /* Two short flashes once a second */
    0B00010101, /* Three short flashes once a second */
    0B01010101  /* Frequent short flashes (4 times per second) */
};
uint8_t blink_loop = 0;
uint8_t blink_mode = 0;

void set_led_mode(led_mode_t mode)
{
  blink_mode = modes[mode];
}

void task_led(void *pvParameters)
{
  gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

  set_led_mode(LED_TWO_BLINK);

  while (1) {
    if( blink_mode & 1<<(blink_loop & 0x07) ) {
      gpio_set_level(GPIO_NUM_2, 0);
    } else {
      gpio_set_level(GPIO_NUM_2, 1);

    }
    if ((blink_loop & 0x07) == 7) {
      /* pause */
      vTaskDelay(5000 / portTICK_RATE_MS);
    }
    blink_loop++;
    vTaskDelay(125 / portTICK_RATE_MS);
  }
}