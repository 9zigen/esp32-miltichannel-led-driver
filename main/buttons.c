/***
** Created by Aleksey Volkov on 16.01.2020.
***/

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

#include "board.h"
#include "light.h"
#include "buttons.h"

static const char *TAG = "BUTTONS";
static QueueHandle_t xQueueButton = NULL;

static void gpio_isr_handler(void *arg)
{
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(xQueueButton, &gpio_num, NULL);
}

static void task_buttons(void *arg)
{
  uint32_t io_num;
  uint8_t active_preset = 0;
  uint8_t brightness_preset[4] = {0, 25, 50, 75};

#ifdef CUSTOM_3CH_CONTROLLER
  /* Power ON/OFF button */
  uint8_t power_state = 1;
#endif


  for (;;) {
    if (xQueueReceive(xQueueButton, &io_num, portMAX_DELAY)) {
      ESP_LOGI(TAG, "GPIO[%lu] intr, val: %d\n", io_num, gpio_get_level(io_num));

      /* button was released after press event */
      if (io_num == GPIO_NUM_0)
      {
        active_preset++;
        if (active_preset == 4) {
          active_preset = 0;
        }

        set_brightness(brightness_preset[active_preset], 0);
        ESP_LOGI(TAG, "set brightness preset: %d", brightness_preset[active_preset]);
      }

#ifdef CUSTOM_3CH_CONTROLLER
      if (io_num == GPIO_NUM_36)
      {
        if (gpio_get_level(GPIO_NUM_36) == 0) {
          vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        if (gpio_get_level(GPIO_NUM_36) == 0) {
          if (power_state) {
            ESP_LOGI(TAG, "power off...");
            power_state = 0;
            set_brightness(0);
          } else {
            ESP_LOGI(TAG, "power on...");
            power_state = 1;
            set_brightness(30);
          }
        }
      }
#endif

      vTaskDelay(200/portTICK_PERIOD_MS);
    }
  }
}

void init_buttons()
{
  /* GPIO_0 */
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);

  /* change gpio intrrupt type for one pin */
  gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_POSEDGE);

#ifdef CUSTOM_3CH_CONTROLLER
  /* Power Button */
  gpio_set_direction(GPIO_NUM_35, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_35, 0);
  gpio_set_direction(GPIO_NUM_36, GPIO_MODE_INPUT);
  gpio_set_pull_mode(GPIO_NUM_36, GPIO_PULLUP_ONLY);
  gpio_set_intr_type(GPIO_NUM_36, GPIO_INTR_POSEDGE);
#endif

  ESP_LOGI(TAG, "[APP] free memory before start buttons event task %lu bytes", esp_get_free_heap_size());

  /* create a queue to handle gpio event from isr */
  xQueueButton = xQueueCreate(1, sizeof(uint32_t));

  /* start gpio task */
  xTaskCreate(task_buttons, "task_buttons", 2048, NULL, 10, NULL);

  /* install gpio isr service */
  gpio_install_isr_service(0);

  /* hook isr handler for specific gpio pin */
  gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void *) GPIO_NUM_0);

#ifdef CUSTOM_3CH_CONTROLLER
  /* Power button isr */
  gpio_isr_handler_add(GPIO_NUM_36, gpio_isr_handler, (void *) GPIO_NUM_36);
#endif
}

