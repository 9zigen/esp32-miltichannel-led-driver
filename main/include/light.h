#include <sys/cdefs.h>
/***
** Created by Aleksey Volkov on 22.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_LIGHT_H
#define HV_CC_LED_DRIVER_RTOS_LIGHT_H

#include <esp_system.h>
#include "board.h"
#include "app_settings.h"

typedef enum {
  GAMMA_1_00 = 100,
  GAMMA_1_80 = 180,
  GAMMA_2_00 = 200,
  GAMMA_2_20 = 220,
  GAMMA_2_40 = 240
} gamma_table_t;

typedef struct {
  double current_duty;
  double real_duty;
  uint32_t steps_left;
  double power_factor;
} led_schedule_t;

typedef struct {
  uint8_t duty[MAX_LED_CHANNELS];
  uint8_t brightness;
  uint8_t magic;
} led_schedule_rtc_mem_t;

typedef struct {
  double target_duty[MAX_LED_CHANNELS];
  double target_brightness;
  uint32_t fade_time;
  uint8_t not_sync;
} x_light_message_t;

/* public */
uint8_t get_brightness();
uint8_t get_light_state();
uint8_t get_channel_duty(uint8_t id);
uint8_t get_channel_state(uint8_t id);
void set_brightness(uint8_t target_brightness, uint8_t not_sync);
void set_channel_duty(uint8_t id, uint8_t duty, uint8_t not_sync);
void set_channel_state(uint8_t id, uint8_t state);
void set_light(const double *target_duty, double target_brightness, uint8_t stop_schedule, uint8_t not_sync);
uint8_t light_is_on();
bool get_schedule_status();
void set_schedule_status(uint8_t on);

_Noreturn void task_light(void *pvParameter);
void task_light_transition(void *pvParameter);

#endif //HV_CC_LED_DRIVER_RTOS_LIGHT_H
