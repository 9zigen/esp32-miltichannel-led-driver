/***
** Created by Aleksey Volkov on 22.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_LIGHT_H
#define HV_CC_LED_DRIVER_RTOS_LIGHT_H

#include "settings.h"

typedef struct {
  double current_duty;
  double real_duty;
  uint32_t steps_left;
} led_schedule_t;

typedef struct {
  double target_duty[MAX_LED_CHANNELS];
  double target_brightness;
  uint32_t steps_left;
} x_light_message_t;

/* public */
uint8_t get_brightness();
uint8_t get_channel_duty(uint8_t id);
uint8_t get_channel_state(uint8_t id);
void set_brightness(uint8_t new_brightness);
void set_channel_duty(uint8_t id, uint8_t duty);
void set_channel_state(uint8_t id, uint8_t state);
void set_light(const uint8_t * target_duty, uint8_t target_brightness);

void task_light(void *pvParameter);
void task_light_transition(void *pvParameter);

#endif //HV_CC_LED_DRIVER_RTOS_LIGHT_H
