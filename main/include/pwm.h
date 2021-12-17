//
// Created by Aleksey Volkov on 19.11.2020.
//

#ifndef ESP32_CC_LED_DRIVER_RTOS_PWM_H
#define ESP32_CC_LED_DRIVER_RTOS_PWM_H

void init_ledc();
void ledc_fade(uint8_t channel, uint32_t duty, uint32_t steps);
void ledc_fade_ms(uint8_t channel, uint32_t duty, uint32_t max_time_ms);
void ledc_set(uint8_t channel, uint32_t duty);
void ledc_fan_set(uint32_t duty);
uint32_t ledc_fan_get();
#endif //ESP32_CC_LED_DRIVER_RTOS_PWM_H
