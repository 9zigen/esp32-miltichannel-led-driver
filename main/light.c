/***
** Created by Aleksey Volkov on 22.12.2019.
***/

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>
#include <stmdriver.h>

#include "settings.h"
#include "light.h"

static const char *TAG = "LIGHT";

/* current led channel state */
volatile led_schedule_t _channel[MAX_LED_CHANNELS] = {0};
volatile uint8_t brightness = 0;

QueueHandle_t xQueueTransition;

static int minutes_left(uint8_t hour, uint8_t minute, uint8_t schedule_hour, uint8_t schedule_minute)
{
  return (schedule_hour - hour) * 60 + schedule_minute - minute;
}

uint8_t get_brightness()
{
  return brightness;
}

uint8_t get_channel_duty(uint8_t id)
{
  if (id >= MAX_LED_CHANNELS)
    return 0;

  return _channel[id].current_duty;
}

uint8_t get_channel_state(uint8_t id)
{
  if (id >= MAX_LED_CHANNELS)
    return 0;

  if (_channel[id].current_duty > 0)
  {
    return 1;
  }
  return 0;
}

void set_channel_state(uint8_t id, uint8_t state)
{
  if (id >= MAX_LED_CHANNELS)
    return;

  if (state == 1)
  {
    set_channel_duty(id, MAX_DUTY);
  } else {
    set_channel_duty(id, 0);
  }
}

void set_channel_duty(uint8_t id, uint8_t duty)
{
  if (id >= MAX_LED_CHANNELS)
    return;

//  x_light_message_t xLightMessage = {0};
//
//  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
//  {
//    /* prepare new queue message */
//    xLightMessage.target_duty[i] = _channel[i].current_duty;
//    if (id == i)
//    {
//      xLightMessage.target_duty[i] = duty;
//    }
//  }
//
//  /* transition duration steps_left * step duration; 50 * 10ms = 500ms */
//  xLightMessage.steps_left = 50;
//  xLightMessage.target_brightness = brightness;
//
//  /* send new queue */
//  //xQueueSendToBack(xQueueTransition, &xLightMessage, 100/portTICK_RATE_MS);
//  print_queue_message(&xLightMessage);
}

void set_brightness(uint8_t new_brightness)
{

  x_light_message_t txMessage;
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* prepare new queue message */
    txMessage.target_duty[i] = _channel[i].current_duty;
  }

  /* transition duration steps_left * step duration; 50 * 10ms = 500ms */
  txMessage.steps_left = 50;
  txMessage.target_brightness = new_brightness;

  /* send new queue */
  xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
  //print_queue_message(&xLightMessage);
}

void set_light(const uint8_t * target_duty, uint8_t target_brightness)
{
  x_light_message_t txMessage;
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* prepare new queue message */
    txMessage.target_duty[i] = target_duty[i];
  }

  /* transition duration steps_left * step duration; 50 * 50ms = 2500ms */
  txMessage.steps_left = 50;
  txMessage.target_brightness = target_brightness;

  /* send new queue */
  xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
}

static void restore_last_pwm()
{
  uint8_t stm_channels[MAX_LED_CHANNELS];
  uint8_t target_duty[MAX_LED_CHANNELS];

  /* restore brightness */
  uint8_t new_brightness = 0;
  get_stm_brightness(&new_brightness);
  ESP_LOGD(TAG, "get_stm_brightness: %u", new_brightness);

  /* get all pwm channels from stm in array */
  get_stm_pwm((uint8_t *)&stm_channels);

  for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
    double current_duty = (stm_channels[i] + 0.0) / (new_brightness / 100.0);
    target_duty[i] = (uint8_t)(current_duty + 0.5);

    ESP_LOGD(TAG, "stm_channels: %u %u", i, stm_channels[i]);
    ESP_LOGD(TAG, "current_duty: %f", _channel[i].current_duty);
  }

  /* restore light */
  set_light((uint8_t*)&target_duty, new_brightness);
}

/* main light control task */
void task_light(void *pvParameter)
{
  time_t now;
  struct tm timeinfo;

  x_light_message_t txMessage;

  /* light transition queue */
  xQueueTransition = xQueueCreate( 10, sizeof( x_light_message_t ) );

  /* load current pwm values from stm */
  restore_last_pwm();

  while (1)
  {
    /* get current local time */
    time(&now);
    localtime_r(&now, &timeinfo);

    /* schedule ---> */
    for (uint8_t j = 0; j < MAX_SCHEDULE; ++j)
    {
      schedule_t *_schedule = get_schedule_config(j);

      /* only enable schedule process */
      if (_schedule->active)
      {
        int munutes_left = minutes_left(timeinfo.tm_hour, timeinfo.tm_min, _schedule->time_hour, _schedule->time_minute);

        ESP_LOGI(TAG, "Time NOW: %d:%d Next Point: %d:%d Minutes left: %d", timeinfo.tm_hour, timeinfo.tm_min,
                 _schedule->time_hour, _schedule->time_minute, munutes_left);

        /* Set target duty from schedule */
        if (munutes_left == 0)
        {
          for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
          {
            /* prepare new queue message */
            txMessage.target_duty[i] = _schedule->duty[i];
          }

          /* transition duration steps_left * step duration; 50 * 10ms = 500ms */
          txMessage.steps_left = 50;
          txMessage.target_brightness = _schedule->brightness;

          /* send new queue */
          xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
          //print_queue_message(xLightMessage);
        }
      }

    }

    /* delay one min */
    vTaskDelay(1000 * 60/portTICK_RATE_MS);
  }
}

/* Update channel state, color transition process schedule end manual changed */
void task_light_transition(void *pvParameter)
{
  double difference = 0;
  double target_real_duty = 0;
  double target_brightness = 0;

  x_light_message_t rxMessage;

  if( xQueueTransition != 0 )
  {
    for(;;) {

      if (xQueueReceive(xQueueTransition, &rxMessage, portMAX_DELAY)) {
        /* new transition received, process all channels together */
        uint32_t steps_left = rxMessage.steps_left;
        target_brightness = rxMessage.target_brightness;

        ESP_LOGI(TAG, "new queue target_brightness: %f", rxMessage.target_brightness);
        for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
        {
          ESP_LOGI(TAG, "new queue ch: %d target_duty: %f current: %f", i, rxMessage.target_duty[i], _channel[i].current_duty);
        }

        while (steps_left > 0) {
          /* process all channels together */
          for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
          {
            /* Calc New Current Duty */
            difference = (rxMessage.target_duty[i] - _channel[i].current_duty) / (_channel[i].steps_left + 1.0);
            _channel[i].current_duty = (float) fabs(_channel[i].current_duty + difference);

            /* Calc New Real Duty, brightness compensated */
            target_real_duty = _channel[i].current_duty * target_brightness / MAX_BRIGHTNESS;

            /* Brightness / Duty changed */
            if (_channel[i].real_duty != target_real_duty) {
              difference = (target_real_duty - _channel[i].real_duty) / (steps_left + 1.0);
              _channel[i].real_duty = (float) fabs(_channel[i].real_duty + difference + 0.5);

              set_stm_pwm_single(i, (uint8_t)_channel[i].real_duty);
            }
          }
          steps_left--;
          vTaskDelay(50 / portTICK_RATE_MS);

        };

        /* end transition */
        for (int j = 0; j < MAX_LED_CHANNELS; ++j) {
          _channel[j].current_duty = rxMessage.target_duty[j];
          target_real_duty = _channel[j].current_duty * target_brightness / MAX_BRIGHTNESS;

          if (_channel[j].real_duty != target_real_duty) {
            _channel[j].real_duty = _channel[j].current_duty * target_brightness / MAX_BRIGHTNESS;

            set_stm_pwm_single(j, (uint8_t)_channel[j].real_duty);
          }
        }
        brightness = target_brightness;

        /* backup brightness */
        uint8_t new_brightness = (uint8_t) target_brightness;
        set_stm_brightness(&new_brightness);

        ESP_LOGI(TAG, "queue done! steps: %d for %d ms.", rxMessage.steps_left, rxMessage.steps_left * 50);

      }
    }
  }
}