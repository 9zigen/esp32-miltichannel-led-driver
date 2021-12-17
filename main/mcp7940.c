/***
** Created by Aleksey Volkov on 22.03.2020.
***/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <esp_log.h>
#include "driver/i2c.h"
#include "board.h"
#include "mcp7940.h"

#if defined(PICO_D4_5CH_LED_DRIVER_AIO) || defined(CUSTOM_7CH_LED_DRIVER_AIO)
#define MCP7940_SDA 27
#define MCP7940_SCL 26
#endif

static const char *TAG="MCP7940";

const uint8_t dev_address = 0x6f;
i2c_port_t i2c_master_port = I2C_NUM_0;

static void i2c_master_init(void)
{
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = MCP7940_SDA;
  conf.scl_io_num = MCP7940_SCL;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = 100000;
  conf.clk_flags = 0;

  ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, I2C_MODE_MASTER, 0, 0, 0));
  ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));

  task_i2cscanner();
}

static uint8_t mcp7940_read_byte(uint8_t reg)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_address<<1), 1);
  i2c_master_write_byte(cmd, reg, 1);

  uint8_t data;
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_address<<1) | 1, 1);
  i2c_master_read_byte(cmd, &data, I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);

  ESP_ERROR_CHECK(i2c_master_cmd_begin(i2c_master_port, cmd, 1000/portTICK_PERIOD_MS));
  i2c_cmd_link_delete(cmd);

  return data;
}

static esp_err_t mcp7940_read_bytes(uint8_t reg, uint8_t * p_data, uint8_t len)
{
  esp_err_t err = ESP_OK;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_address<<1), 1);
  i2c_master_write_byte(cmd, reg, 1);

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_address<<1) | 1, 1);
  i2c_master_read(cmd, p_data, len, I2C_MASTER_LAST_NACK);
  i2c_master_stop(cmd);
  err = i2c_master_cmd_begin(i2c_master_port, cmd, 1000/portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  return err;
}

static esp_err_t mcp7940_write_byte(uint8_t reg, uint8_t value)
{
  esp_err_t ret = ESP_OK;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_address << 1), 1);
  i2c_master_write_byte(cmd, reg, 1);
  i2c_master_write_byte(cmd, value, 1);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000/portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static esp_err_t mcp7940_write_bytes(uint8_t reg, uint8_t *p_data, uint8_t len)
{
  esp_err_t ret = ESP_OK;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (dev_address << 1), 1);
  i2c_master_write_byte(cmd, reg, 1);
  i2c_master_write(cmd, p_data, len, 1);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_master_port, cmd, 1000/portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return ret;
}

static uint8_t read_register(uint8_t reg_address)
{
  return mcp7940_read_byte(reg_address);
}

static void write_register(uint8_t reg_address, uint8_t value)
{
  ESP_ERROR_CHECK(mcp7940_write_byte(reg_address, value));
}

static void clear_bit(uint8_t reg_address, uint8_t bit)
{
  uint8_t reg = read_register(reg_address);
  reg &= ~(1 << bit);
  write_register(reg_address, reg);
}

static void set_bit(uint8_t reg_address, uint8_t bit)
{
  uint8_t reg = read_register(reg_address);
  reg |= 1 << bit;
  write_register(reg_address, reg);
}

static bool is_set_bit(uint8_t reg_address, uint8_t bit)
{
  return read_register(reg_address) & (1 << bit);
}

void mcp7940_init(void)
{
  i2c_master_init();

  /* Check if oscillator is running */
  if (!is_set_bit(MCP7940_RTCWKDAY, MCP7940_OSCRUN))
  {
    /* Start Oscillator */
    set_bit(MCP7940_RTCSEC, MCP7940_ST);

    /* Wait while oscillator start */
    while(!is_set_bit(MCP7940_RTCWKDAY, MCP7940_OSCRUN)) {};
  }

  /* Check if Backup Battery enabled */
  if (!is_set_bit(MCP7940_RTCWKDAY, MCP7940_VBATEN))
  {
    /* VBAT input enable */
    set_bit(MCP7940_RTCWKDAY, MCP7940_VBATEN);
  }
}

void mcp7940_get_datetime(datetime_t *datetime)
{
  uint8_t data[7];

  /* Read REGs 0x00 - 0x06 */
  mcp7940_read_bytes(MCP7940_RTCSEC, data, 7);

  /* Sec */
  uint8_t sec = data[0] & 0xF;            /* Second’s Ones Digit Contains a value from 0 to 9 */
  sec += ((data[0] & 0x70) >> 4) * 10;    /* Second’s Tens Digit Contains a value from 0 to 5 */

  /* Min */
  uint8_t min = data[1] & 0xF;            /* Minute’s Ones Digit Contains a value from 0 to 9 */
  min += ((data[1] & 0x70) >> 4) * 10;    /* Minute’s Tens Digit Contains a value from 0 to 5 */

  /* 12-hour format */
  uint8_t is_12h = data[2] & 0x40? 1:0;   /* true if 12h format */

  /* AM/PM */
  uint8_t is_PM = data[2] & 0x20? 1:0;    /* true if PM */

  /* Hour */
  uint8_t hour = data[2] & 0xF;           /* Hour’s Ones Digit Contains a value from 0 to 9 */
  if (is_12h)
  {
    hour += ((data[2] & 0x10) >> 4) * 10;   /* Hour’s Tens Digit Contains a value from 0 to 1 */
  } else {
    hour += ((data[2] & 0x30) >> 4) * 10;   /* Hour’s Tens Digit Contains a value from 0 to 2 */
  }

  /* Weekday */
  uint8_t weekday = data[3] & 0x7;        /* Weekday Contains a value from 1 to 7. */

  /* Day */
  uint8_t day = data[4] & 0xF;            /* Date’s Ones Digit Contains a value from 0 to 9 */
  day += ((data[4] & 0x30) >> 4) * 10;    /* Date’s Tens Digit Contains a value from 0 to 3 */

  /* Month */
  uint8_t month = data[5] & 0xF;          /* Month’s Ones Digit Contains a value from 0 to 9 */
  month += ((data[5] & 0x10) >> 4) * 10;  /* Month’s Tens Digit Contains a value from 0 to 1 */

  /* Year */
  uint8_t year = data[6] & 0xF;           /* Year’s Ones Digit Contains a value from 0 to 9 */
  year += ((data[6] & 0xF0) >> 4) * 10;   /* Year’s Tens Digit Contains a value from 0 to 9 */

  datetime->year = year;
  datetime->month = month;
  datetime->weekday = weekday;
  datetime->day = day;
  datetime->is_12h = is_12h;
  datetime->is_PM = is_PM;
  datetime->hour = hour;
  datetime->min = min;
  datetime->sec = sec;
}

static uint8_t dec2bcd(uint8_t num)
{
  uint8_t ones = 0;
  uint8_t tens = 0;
  uint8_t temp = 0;

  ones = num % 10;
  temp = num / 10;
  tens = (temp % 10) << 4;
  return (tens + ones);
}

void mcp7940_set_datetime(datetime_t *datetime)
{
  uint8_t data[8];

  /* Read REGs 0x00 - 0x06 */
  mcp7940_read_bytes(MCP7940_RTCSEC, data, 7);

  /* Check if oscillator is running */
  if (!is_set_bit(MCP7940_RTCWKDAY, MCP7940_OSCRUN))
  {
    /* Stop Oscillator */
    clear_bit(MCP7940_RTCSEC, MCP7940_ST);

    /* Wait while oscillator stop */
    while(is_set_bit(MCP7940_RTCWKDAY, MCP7940_OSCRUN)) {};
  }

  /* Start REG */
  /* Sec */
  data[0] &= 0x80;                        /* clear current time val */
  data[0] |= dec2bcd(datetime->sec);      /* Second’s BCD format */

  /* Min */
  data[1] &= 0x80;                        /* clear current time val */
  data[1] |= dec2bcd(datetime->min);      /* Minute’s BCD format */

  /* AM/PM if 12-hour format */
  if (datetime->is_12h && datetime->is_PM)
  {
    data[2] |= 0x40;                      /* set 12h bit*/
    data[2] |= 0x20;                      /* set PM bit*/
  }
  else if (datetime->is_12h && !datetime->is_PM)
  {
    data[2] |= 0x40;                      /* set 12h bit */
    data[2] &= ~0x20;                     /* clear PM bit, set AM */
  } else {
    data[2] &= ~0x40;                     /* clear 12h bit, set 24h format */
    data[2] &= ~0x20;                     /* clear PM bit, used as hour */
  };

  /* Hour */
  data[2] &= 0xE0;                        /* clear current time val */
  data[2] |= dec2bcd(datetime->hour);     /* Hour’s BCD format */

  /* Weekday */
  data[3] &= 0xF8;                        /* clear current time val */
  data[3] |= dec2bcd(datetime->weekday);  /* Weekday BCD format */

  /* Day */
  data[4] &= 0xC0;                        /* clear current time val */
  data[4] |= dec2bcd(datetime->day);      /* Date’s BCD format */

  /* Month */
  data[5] &= 0xE0;                        /* clear current time val */
  data[5] |= dec2bcd(datetime->month);    /* Month’s BCD format */

  /* Year */
  data[6] &= 0x00;                        /* clear current time val */
  data[6] |= dec2bcd(datetime->year);     /* Year’s BCD format */

  mcp7940_write_bytes(MCP7940_RTCSEC, data, 7);

  /* Check if oscillator is running */
  if (!is_set_bit(MCP7940_RTCWKDAY, MCP7940_OSCRUN))
  {
    /* Start Oscillator */
    set_bit(MCP7940_RTCSEC, MCP7940_ST);

    /* Wait while oscillator start */
    while(!is_set_bit(MCP7940_RTCWKDAY, MCP7940_OSCRUN)) {};
  }
}

esp_err_t mcp7940_read_ram(uint8_t offset, uint8_t *buf, uint8_t len)
{
  if (offset + len > MCP7940_RAM_BYTES)
    return ESP_ERR_NO_MEM;

  return mcp7940_read_bytes(MCP7940_RAM_ADDRESS + offset, buf, len);
}

esp_err_t mcp7940_write_ram(uint8_t offset, uint8_t *buf, uint8_t len)
{
  if (offset + len > MCP7940_RAM_BYTES)
    return ESP_ERR_NO_MEM;

  return mcp7940_write_bytes(MCP7940_RAM_ADDRESS + offset, buf, len);
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