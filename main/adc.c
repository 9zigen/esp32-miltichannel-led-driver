//
// Created by Aleksey Volkov on 17.11.2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#include "board.h"
#include "adc.h"

#if defined(PICO_D4_5CH_LED_DRIVER_AIO) || defined(CUSTOM_7CH_LED_DRIVER_AIO)
#define NTC_ADC ADC1_CHANNEL_4  /* 32XP */
#define VCC_ADC ADC1_CHANNEL_5  /* 32XN */
#endif

/* Voltage Divider Calculation
 * Vs = 50v
 * Vo = 3.3v (for max utilize adc)
 * Vo = Vs * R2 / (R1 + R2)
 * Vs = Vo * (R1 + R2) / R2
 * Ratio = (Vs * R2 / (R1 + R2)) * 100 / Vo * (R1 + R2) / R2
 * Ratio = 2.71 * 100 / 18.517 = 14.635
 * */

/* VCC Divider ratio 267K/10K */
#define VCC_DEVIDER_RATIO                27.6f /* 26.68 calibrated value if 49v_in 1.77v_adc*/

/* NTC Temperature calculation */
#define BALANCE_RESISTOR      10000
#define BETA                  3950
#define ROOM_TEMP             298.15      /* room temperature in Kelvin 273.15 + 25 */
#define NTC_25T_RESISTANCE    10000       /* NTC resistance when temperature is 25 Celsius */
#define NTC_REF_MV            3300

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static const char *TAG = "ADC_TEMP";
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    printf("Characterized using Two Point Value\n");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    printf("Characterized using eFuse Vref\n");
  } else {
    printf("Characterized using Default Vref\n");
  }
}

void init_adc() {
  adc1_config_width(width);
  adc1_config_channel_atten(NTC_ADC, ADC_ATTEN_DB_11);

  //Characterize ADC1
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, width, DEFAULT_VREF, adc_chars);
  print_char_val_type(val_type);
}

float read_vcc_voltage() {
  uint32_t adc_reading = 0;

  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    adc_reading += adc1_get_raw(VCC_ADC);
  }
  adc_reading /= NO_OF_SAMPLES;

  return (float)esp_adc_cal_raw_to_voltage(adc_reading, adc_chars) * VCC_DEVIDER_RATIO;
}

int16_t read_ntc_temperature() {
  uint32_t adc_reading = 0;

  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    adc_reading += adc1_get_raw(NTC_ADC);
  }
  adc_reading /= NO_OF_SAMPLES;

  /* NTC Disconnected */
  if (adc_reading == 4095) {
    return -273;
  }

  uint32_t ntc_millivolts = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
  double r_thermistor = BALANCE_RESISTOR / (((double)(NTC_REF_MV) / ntc_millivolts) - 1);
  double temperature_kelvin = (BETA * ROOM_TEMP) / (BETA + ROOM_TEMP * log(r_thermistor / NTC_25T_RESISTANCE));
  double temperature_celsius = temperature_kelvin - 273.15;  // convert kelvin to celsius

  ESP_LOGD(TAG, "NTC mv: %u R: %f", ntc_millivolts, r_thermistor);
  /* round */
  return (int16_t)(temperature_celsius + 0.5);
}
