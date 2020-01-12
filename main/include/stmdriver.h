/***
** Created by Aleksey Volkov on 16.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_STMDRIVER_H
#define HV_CC_LED_DRIVER_RTOS_STMDRIVER_H

#include <time.h>
#include "driver/i2c.h"

#define STM_ADDRESS                 0x32    /* STM32 I2C Device address */
#define STM_BRIGHTNESS_REG_SET      0x08    /* SET Brightness Write byte (only for restore on unexpected reason) */
#define STM_BRIGHTNESS_REG_GET      0x09    /* GET Brightness Read byte (only for restore on unexpected reason) */
#define STM_PWM_REG_SET_ALL         0x10    /* SET All PWM Channels: order CH1-CH2---CH8 Write 8x byte */
#define STM_PWM_REG_SET_1           0x11    /* SET PWM Channel 1 Write byte */
#define STM_PWM_REG_SET_2           0x12    /* SET PWM Channel 2 Write byte */
#define STM_PWM_REG_SET_3           0x13    /* SET PWM Channel 3 Write byte */
#define STM_PWM_REG_SET_4           0x14    /* SET PWM Channel 4 Write byte */
#define STM_PWM_REG_SET_5           0x15    /* SET PWM Channel 5 Write byte */
#define STM_PWM_REG_SET_6           0x16    /* SET PWM Channel 6 Write byte */
#define STM_PWM_REG_SET_7           0x17    /* SET PWM Channel 7 Write byte */
#define STM_PWM_REG_SET_8           0x18    /* SET PWM Channel 8 Write byte */
#define STM_PWM_REG_GET_ALL         0x19    /* GET All PWM Channels: order CH1-CH2---CH8 Read  8x byte */
#define STM_TEMP_REG_NTC            0x20    /* NTC Temp sensor: Read  2x byte, MSB first */
#define STM_TEMP_REG_MCU            0x21    /* MCU Temp sensor: Read  2x byte, MSB first */
#define STM_VCC_REG                 0x22    /* Power In Voltage Read  2x byte, MSB first */
#define STM_VBAT_REG                0x23    /* RTC Bat voltage  Read  2x byte, MSB first */
#define STM_RTC_REG_GET_DATE_TIME   0x30    /* Read  7x byte, order Y-M-Wd-D-h-m-s */
#define STM_RTC_REG_SET_DATE_TIME   0x31    /* Write 7x byte, order Y-M-Wd-D-h-m-s */

typedef struct {
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} stm_datetime_t;

void init_stm();
void i2c_master_init();

/* 8 bit REG */
void stm_read_reg8(uint8_t reg, uint8_t * value);
void stm_write_reg8(uint8_t reg, uint8_t * value);

/* load rtc time from stm32 processor */
void get_stm_rtc(stm_datetime_t *date_time);

/* set rtc time in stm32 processor */
void set_stm_rtc(stm_datetime_t *date_time);

/* PWM control */

/* set all channels */
void set_stm_pwm(uint8_t * channel);

/* get all channels */
void get_stm_pwm(uint8_t * channel);

/* set single channel */
void set_stm_pwm_single(uint8_t channel, uint8_t pwm);

/* backup brightness */
void set_stm_brightness(uint8_t *brightness);

/* restore brightness */
void get_stm_brightness(uint8_t *brightness);

/* get ntc temperature in celsius */
uint16_t get_stm_ntc_temperature(void);

/* get mcu internal temperature in celsius */
uint16_t get_stm_mcu_temperature(void);

/* get Power In in millivolts */
uint16_t get_stm_vcc_power();

/* get RTC battery in millivolts */
uint16_t get_stm_vbat();

uint16_t stm_read_reg16(uint8_t reg);
esp_err_t HAL_I2C_Write(i2c_port_t i2c_port, uint8_t address, uint8_t reg, uint8_t *data, size_t length);
esp_err_t HAL_I2C_Read(i2c_port_t i2c_port, uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, size_t length);
void task_i2cscanner();
void get_rtc(struct tm);

#endif //HV_CC_LED_DRIVER_RTOS_STMDRIVER_H
