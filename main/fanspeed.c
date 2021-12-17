//
// Created by Aleksey Volkov on 23.11.2020.
//

#include <math.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_system.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "driver/pcnt.h"

#include "settings.h"
#include "pwm.h"
#include "adc.h"
#include "fanspeed.h"

static const char *TAG = "FANSPEED";

TimerHandle_t xClearCounterTimer = NULL;
int16_t counter = 0;

/* reset retry counter and try to connect tu AP */
void vClearCounterTimerCallback( TimerHandle_t pxTimer )
{
  /* get current value */
  pcnt_get_counter_value(PCNT_UNIT_0, &counter);

  /* clear counter */
  pcnt_counter_clear(PCNT_UNIT_0);
}

static void pcnt_init(void)
{
  /* Prepare configuration for the PCNT unit */
  pcnt_config_t pcnt_config = {
      // Set PCNT input signal and control GPIOs
      .pulse_gpio_num = GPIO_NUM_26,
      .ctrl_gpio_num = PCNT_PIN_NOT_USED,
      .channel = PCNT_CHANNEL_0,
      .unit = PCNT_UNIT_0,
      // What to do on the positive / negative edge of pulse input?
      .pos_mode = PCNT_COUNT_DIS,   // Count up on the positive edge
      .neg_mode = PCNT_COUNT_INC,   // Keep the counter value on the negative edge
  };
  /* Initialize PCNT unit */
  pcnt_unit_config(&pcnt_config);

  /* Configure and enable the input filter */
  pcnt_set_filter_value(PCNT_UNIT_0, 100);
  pcnt_filter_enable(PCNT_UNIT_0);

  /* Enable events on zero, maximum and minimum limit values */
  pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_ZERO);
  pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_H_LIM);
  pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_L_LIM);

  /* Initialize PCNT's counter */
  pcnt_counter_pause(PCNT_UNIT_0);
  pcnt_counter_clear(PCNT_UNIT_0);

  /* Everything is set up, now go to counting */
  pcnt_counter_resume(PCNT_UNIT_0);
}

void fan_speed_init()
{
  /* Initialize PCNT */
  pcnt_init();

  /* clear counter timer */
  xClearCounterTimer = xTimerCreate( "ClearCounterTimer", ( 250 / portTICK_PERIOD_MS), pdTRUE, 0, vClearCounterTimerCallback);

  if( xClearCounterTimer != NULL ) {
    xTimerStart( xClearCounterTimer, 1000 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "xClearCounterTimer started");
  }
}

/* RPM */
uint32_t get_fan_rpm() {
  return counter * 4 * 60;
}
