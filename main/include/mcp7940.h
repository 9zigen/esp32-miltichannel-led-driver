/***
** Created by Aleksey Volkov on 22.03.2020.
***/

#ifndef ESP32_CC_LED_DRIVER_MCP7940_H
#define ESP32_CC_LED_DRIVER_MCP7940_H

#define MCP7940_ADDRESS           0x6F //Device address, fixed value
#define MCP7940_RTCSEC            0x00 //Timekeeping, RTCSEC Register address
#define MCP7940_RTCMIN            0x01 //Timekeeping, RTCMIN Register address
#define MCP7940_RTCHOUR           0x02 //Timekeeping, RTCHOUR Register address
#define MCP7940_RTCWKDAY          0x03 //Timekeeping, RTCWKDAY Register address
#define MCP7940_RTCDATE           0x04 //Timekeeping, RTCDATE Register address
#define MCP7940_RTCMTH            0x05 //Timekeeping, RTCMTH Register address
#define MCP7940_RTCYEAR           0x06 //Timekeeping, RTCYEAR Register address
#define MCP7940_CONTROL           0x07 //Timekeeping, RTCCONTROL Register address
#define MCP7940_OSCTRIM           0x08 //Timekeeping, RTCOSCTRIM Register address
#define MCP7940_ALM0SEC           0x0A //Alarm 0, ALM0SEC Register address
#define MCP7940_ALM0MIN           0x0B //Alarm 0, ALM0MIN Register address
#define MCP7940_ALM0HOUR          0x0C //Alarm 0, ALM0HOUR Register address
#define MCP7940_ALM0WKDAY         0x0D //Alarm 0, ALM0WKDAY Register address
#define MCP7940_ALM0DATE          0x0E //Alarm 0, ALM0DATE Register address
#define MCP7940_ALM0MTH           0x0F //Alarm 0, ALM0MTH Register address
#define MCP7940_ALM1SEC           0x11 //Alarm 1, ALM1SEC Register address
#define MCP7940_ALM1MIN           0x12 //Alarm 1, ALM1MIN Register address
#define MCP7940_ALM1HOUR          0x13 //Alarm 1, ALM1HOUR Register address
#define MCP7940_ALM1WKDAY         0x14 //Alarm 1, ALM1WKDAY Register address
#define MCP7940_ALM1DATE          0x15 //Alarm 1, ALM1DATE Register address
#define MCP7940_ALM1MTH           0x16 //Alarm 1, ALM1MONTH Register address
#define MCP7940_PWRDNMIN          0x18 //Power-Fail, PWRDNMIN Register address
#define MCP7940_PWRDNHOUR         0x19 //Power-Fail, PWRDNHOUR Register address
#define MCP7940_PWRDNDATE         0x1A //Power-Fail, PWDNDATE Register address
#define MCP7940_PWRDNMTH          0x1B //Power-Fail, PWRDNMTH Register address
#define MCP7940_PWRUPMIN          0x1C //Power-Fail, PWRUPMIN Register address
#define MCP7940_PWRUPHOUR         0x1D //Power-Fail, PWRUPHOUR Register address
#define MCP7940_PWRUPDATE         0x1E //Power-Fail, PWRUPDATE Register address
#define MCP7940_PWRUPMTH          0x1F //Power-Fail, PWRUPMTH Register address
#define MCP7940_RAM_ADDRESS       0x20 //NVRAM - Start address for SRAM
#define MCP7940_RAM_BYTES         64   //NVRAM - Bytes
#define MCP7940_ST                   7 //MCP7940 register bits. RTCSEC reg
#define MCP7940_12_24                6 //RTCHOUR, PWRDNHOUR & PWRUPHOUR
#define MCP7940_AM_PM                5 //RTCHOUR, PWRDNHOUR & PWRUPHOUR
#define MCP7940_OSCRUN               5 //RTCWKDAY register
#define MCP7940_PWRFAIL              4 //RTCWKDAY register
#define MCP7940_VBATEN               3 //RTCWKDAY register
#define MCP7940_LPYR                 5 //RTCMTH register
#define MCP7940_OUT                  7 //CONTROL register
#define MCP7940_SQWEN                6 //CONTROL register
#define MCP7940_ALM1EN               5 //CONTROL register
#define MCP7940_ALM0EN               4 //CONTROL register
#define MCP7940_EXTOSC               3 //CONTROL register
#define MCP7940_CRSTRIM              2 //CONTROL register
#define MCP7940_SQWFS1               1 //CONTROL register
#define MCP7940_SQWFS0               0 //CONTROL register
#define MCP7940_SIGN                 7 //OSCTRIM register
#define MCP7940_ALMPOL               7 //ALM0WKDAY register
#define MCP7940_ALM0IF               3 //ALM0WKDAY register
#define MCP7940_ALM1IF               3 //ALM1WKDAY register
#define SECONDS_PER_DAY              86400 //60 secs * 60 mins * 24 hours
#define SECONDS_FROM_1970_TO_2000    946684800 //Seconds between year 1970 and 2000

typedef struct {
  uint8_t year;
  uint8_t month;
  uint8_t weekday;
  uint8_t day;
  uint8_t is_12h;
  uint8_t is_PM;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
} datetime_t ;

void mcp7940_init(void);
void mcp7940_get_datetime(datetime_t *datetime);
void mcp7940_set_datetime(datetime_t *datetime);
esp_err_t mcp7940_read_ram(uint8_t offset, uint8_t *buf, uint8_t len);
esp_err_t mcp7940_write_ram(uint8_t offset, uint8_t *buf, uint8_t len);
void task_i2cscanner();

#endif //ESP32_CC_LED_DRIVER_MCP7940_H
