//
// Created by Aleksey Volkov on 17.11.2020.
//

#ifndef ESP32_CC_LED_DRIVER_RTOS_ADC_H
#define ESP32_CC_LED_DRIVER_RTOS_ADC_H

void init_adc();
void init_adc2();
void init_temp_sensor();

float read_vcc_voltage();
int16_t read_ntc_temperature();
int16_t read_mcu_temperature();
float read_mcu_temperature_float();

#endif //ESP32_CC_LED_DRIVER_RTOS_ADC_H
