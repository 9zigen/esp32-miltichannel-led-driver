/***
** Created by Aleksey Volkov on 16.12.2019.
***/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <esp_log.h>
#include "driver/i2c.h"
#include "stmdriver.h"

#define STM32_SDA 4
#define STM32_SCL 5

static const char *TAG="STMDRIVER";

void init_stm()
{
  ESP_LOGI(TAG, "[APP] free memory before start stm driver %d bytes", esp_get_free_heap_size());

  /* BOOT0 */
//  gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
//  gpio_set_level(GPIO_NUM_12, 0);

  /* RESET */
//  gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
//  gpio_set_level(GPIO_NUM_13, 1);
//  vTaskDelay(100);
//  gpio_set_level(GPIO_NUM_13, 0);
//  vTaskDelay(10);

  i2c_master_init();

  task_i2cscanner();
}

/**
 * @brief i2c master initialization
 */
void i2c_master_init()
{
  int i2c_master_port = I2C_NUM_0;
  i2c_config_t conf;

  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = STM32_SDA;
  conf.scl_io_num = STM32_SCL;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  //conf.clk_stretch_tick = 0; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.

  ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode));
  ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
}

/* load rtc time from stm32 processor */
void get_stm_rtc(stm_datetime_t *date_time)
{
  uint8_t rx_data[7];
  for (int i = 0; i < 7; ++i) {
    rx_data[i] = 0;
  }

  HAL_I2C_Read(I2C_NUM_0, STM_ADDRESS, STM_RTC_REG_GET_DATE_TIME, rx_data, 7);

  date_time->year = rx_data[0];
  date_time->month = rx_data[1];
  date_time->day = rx_data[2];
  date_time->weekday = rx_data[3];
  date_time->hour = rx_data[4];
  date_time->minute = rx_data[5];
  date_time->second = rx_data[6];

  if (date_time->month == 0) {
    date_time->month = 1;
  }
}

/* set rtc time in stm32 processor */
void set_stm_rtc(stm_datetime_t *date_time)
{
  uint8_t tx_data[7];
  tx_data[0] = date_time->year;
  tx_data[1] = date_time->month;
  tx_data[2] = date_time->day;
  tx_data[3] = date_time->weekday;
  tx_data[4] = date_time->hour;
  tx_data[5] = date_time->minute;
  tx_data[6] = date_time->second;

  HAL_I2C_Write(I2C_NUM_0, STM_ADDRESS, STM_RTC_REG_SET_DATE_TIME, tx_data, 7);
}

/* set pwm */
void set_stm_pwm(uint8_t * channel)
{
  HAL_I2C_Write(I2C_NUM_0, STM_ADDRESS, STM_PWM_REG_SET_ALL, channel, 8);
}

void set_stm_pwm_single(uint8_t channel, uint8_t pwm)
{
  HAL_I2C_Write(I2C_NUM_0, STM_ADDRESS, STM_PWM_REG_SET_1 + channel, &pwm, 1);
}

void get_stm_pwm(uint8_t * channel)
{
  HAL_I2C_Read(I2C_NUM_0, STM_ADDRESS, STM_PWM_REG_GET_ALL, channel, 8);
}

void set_stm_brightness(uint8_t * brightness)
{
  HAL_I2C_Write(I2C_NUM_0, STM_ADDRESS, STM_BRIGHTNESS_REG_SET, brightness, 1);
}

void get_stm_brightness(uint8_t * brightness)
{
  stm_read_reg8(STM_BRIGHTNESS_REG_GET, brightness);
}

int16_t get_stm_ntc_temperature()
{
  return stm_read_reg16(STM_TEMP_REG_NTC) - 255;
}

int16_t get_stm_mcu_temperature()
{
  return stm_read_reg16(STM_TEMP_REG_MCU) - 255;
}

uint16_t get_stm_vcc_power()
{
  return stm_read_reg16(STM_VCC_REG);
}

uint16_t get_stm_vbat()
{
  return stm_read_reg16(STM_VBAT_REG);
}

uint8_t get_stm_status()
{
  uint8_t status;
  stm_read_reg8(STM_STATUS_REG, &status);
  return status;
}

void stm_read_reg8(uint8_t reg, uint8_t * value)
{
  HAL_I2C_Read(I2C_NUM_0, STM_ADDRESS, reg, value, 1);
}

void stm_write_reg8(uint8_t reg, uint8_t * value)
{
  HAL_I2C_Write(I2C_NUM_0, STM_ADDRESS, reg, value, 1);
}

uint16_t stm_read_reg16(uint8_t reg)
{
  uint8_t rxData[2] = {0};
  HAL_I2C_Read(I2C_NUM_0, STM_ADDRESS, reg, rxData, 2);
  return (uint16_t) (rxData[0] << 8 | rxData[1]);
}

void stm32_write_reg16(uint8_t reg, uint16_t data)
{
  HAL_I2C_Read(I2C_NUM_0, STM_ADDRESS, reg, (uint8_t*)&data, 2);
}

esp_err_t HAL_I2C_Write(i2c_port_t i2c_port, uint8_t address, uint8_t reg, uint8_t *data, size_t length) {

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( address << 1 ), true);
  i2c_master_write_byte(cmd, reg, true);
  i2c_master_write(cmd, data, length, true);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd,  1000/portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

esp_err_t HAL_I2C_Read(i2c_port_t i2c_port, uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data, size_t length) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, ( dev_addr << 1 ) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd, reg_addr, true);

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_addr << 1) | I2C_MASTER_READ, true);

  if (length > 1) {
    i2c_master_read(cmd, reg_data, length - 1, I2C_MASTER_ACK);
  }
  i2c_master_read_byte(cmd, reg_data + length - 1, I2C_MASTER_NACK);
  i2c_master_stop(cmd);

  esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd,  20/portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

void task_i2cscanner() {
  ESP_LOGD(TAG, ">> i2cScanner");
  int i;
  esp_err_t espRc;
  printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
  printf("00:         ");
  for (i=3; i< 0x78; i++) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
    i2c_master_stop(cmd);

    espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
    if (i%16 == 0) {
      printf("\n%.2x:", i);
    }
    if (espRc == 0) {
      printf(" %.2x", i);
    } else {
      printf(" --");
    }
    //ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
    i2c_cmd_link_delete(cmd);
  }
  printf("\n");
  //vTaskDelete(NULL);
}