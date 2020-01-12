/***
** Created by Aleksey Volkov on 07.01.2020.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_LED_H
#define HV_CC_LED_DRIVER_RTOS_LED_H


typedef enum {
  LED_OFF,
  LED_ON,
  LED_SLOW_BLINK,
  LED_FAST_BLINK,
  LED_TWO_BLINK,
  LED_THREE_BLINK,
  LED_FOUR_BLINK
} led_mode_t;

void set_led_mode(led_mode_t mode);

void task_led(void *pvParameters);

#endif //HV_CC_LED_DRIVER_RTOS_LED_H
