//
// Created by Aleksey Volkov on 23.11.2020.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <math.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_system.h>
#include "string.h"

#include "esp_system.h"
#include "esp_timer.h"

#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <driver/pulse_cnt.h>

#include "app_settings.h"
#include "pwm.h"
#include "adc.h"
#include "fanspeed.h"

static const char *TAG = "FANSPEED";

esp_timer_handle_t calc_speed_timer;
pcnt_unit_handle_t pcnt_unit = NULL;
int counter = 0;

/* reset retry counter and try to connect tu AP */
void calc_speed_timer_callback(void* arg)
{
  /* get current value */
  ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &counter));

  /* clear counter */
  ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
  ESP_ERROR_CHECK(esp_timer_start_once(calc_speed_timer, 2500000));
}

static void pcnt_init(void)
{
  ESP_LOGI(TAG, "install pcnt unit");
  pcnt_unit_config_t unit_config = {
      .high_limit = 65535,
      .low_limit = -1,
  };

  ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

  ESP_LOGI(TAG, "set glitch filter");
  pcnt_glitch_filter_config_t filter_config = {
      .max_glitch_ns = 1000,
  };
  ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

  ESP_LOGI(TAG, "install pcnt channels");
  pcnt_chan_config_t chan_a_config = {
      .edge_gpio_num = GPIO_NUM_5,
      .level_gpio_num = -1,
  };
  pcnt_channel_handle_t pcnt_chan_a = NULL;
  ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

  ESP_LOGI(TAG, "set edge and level actions for pcnt channels");
  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));

  ESP_LOGI(TAG, "clear pcnt unit");
  ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
  ESP_LOGI(TAG, "start pcnt unit");
  ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}

void fan_speed_init()
{
  /* Initialize PCNT */
  pcnt_init();

  /* calc speed/clear counter timer */
  const esp_timer_create_args_t timer_args = {
      .callback = &calc_speed_timer_callback,
      .name = "calc_speed_timer"
  };
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &calc_speed_timer));
  ESP_ERROR_CHECK(esp_timer_start_once(calc_speed_timer, 2500000));
}

/* RPM */
uint32_t get_fan_rpm() {
  return counter * 4 * 60;
}
