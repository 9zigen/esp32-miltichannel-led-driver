/***
** Created by Aleksey Volkov on 19.12.2019.
***/
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/event_groups.h>
#include <esp_timer.h>
#include <mcp7940.h>


#include "sntp.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lwip/apps/sntp.h"

#include "rtc.h"
#include "main.h"
#include "board.h"
#include "connect.h"
#include "app_settings.h"

static const char *TAG = "RTC";

uint8_t ntp_sync = 0;

static void time_sync_notification_cb(struct timeval *tv)
{
  ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(services_t * config)
{
  ESP_LOGI(TAG, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, config->ntp_server);
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  sntp_init();
}

static void obtain_time(void)
{
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, 1000 * 30 / portTICK_PERIOD_MS);

  services_t * services = get_services();

  if (services->enable_ntp && strlen(services->ntp_server) > 5)
  {
    initialize_sntp(services);
  }

  // wait for time to be set
  time_t now = 0;
  struct tm timeinfo = { 0 };
  int retry = 0;
  const int retry_count = 10;

  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

  time(&now);
  localtime_r(&now, &timeinfo);

  if (retry < retry_count) {
    ntp_sync = 1;
  }
}

/* Get time from stm32 rtc */
//void sync_time()
//{
//  struct tm timeinfo;
//  datetime_t datetime;
//
//  /* load config */
//  services_t * services = get_services();
//
//  if (services->enable_ntp && ntp_sync) {
//    ESP_LOGI(TAG, "NTP Sync`d, sync MCP7940");
//
//  }
//
//  mcp7940_get_datetime(&datetime);
//
//  timeinfo.tm_year = datetime.year + 2000 - 1900;  /* stm return only last two dig of year */
//  timeinfo.tm_mon  = datetime.month - 1;
//  timeinfo.tm_mday = datetime.day;
//  timeinfo.tm_wday = datetime.weekday;
//  timeinfo.tm_hour = datetime.hour;
//  timeinfo.tm_min  = datetime.min;
//  timeinfo.tm_sec  = datetime.sec;
//
//  time_t stm_time = mktime(&timeinfo);
//  struct timeval stm_now = { .tv_sec = stm_time };
//
//  /* Set time from STM RTC */
//  settimeofday(&stm_now, NULL);
//}

void init_clock()
{
  time_t now;
  struct tm timeinfo;
  char strftime_buf[64];
  char tz_buff[32];

  /* load config */
  services_t * services = get_services();

  /* Set timezone from config */
  if (services->utc_offset > 0) {
    snprintf(tz_buff, 32, "UTC-%d", services->utc_offset + ((uint8_t)services->ntp_dst));    //GMT+2
  } else {
    uint8_t offset = fabs((double)services->utc_offset);
    snprintf(tz_buff, 32, "UTC+%d",  offset + ((uint8_t)services->ntp_dst));    //UTC-2
  }
  setenv("TZ", tz_buff, 1);
  tzset();
  ESP_LOGI(TAG, "new TZ is: %s", getenv("TZ"));


#ifdef USE_RTC
  /* Get time from mcp7940 rtc */
  datetime_t datetime;
  mcp7940_get_datetime(&datetime);

  timeinfo.tm_year = datetime.year + 2000 - 1900;  /* stm return only last two dig of year */
  timeinfo.tm_mon  = datetime.month - 1;
  timeinfo.tm_mday = datetime.day;
  timeinfo.tm_wday = datetime.weekday;
  timeinfo.tm_hour = datetime.hour;
  timeinfo.tm_min  = datetime.min;
  timeinfo.tm_sec  = datetime.sec;

  time_t stm_time = mktime(&timeinfo);
  struct timeval stm_now = { .tv_sec = stm_time };

  /* Set time from STM RTC */
  settimeofday(&stm_now, NULL);
#endif

  /* Print time from STM RTC */
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);

  // Is time set? If not, tm_year will be (1970 - 1900).
  if (timeinfo.tm_year < (2020 - 1900)) {
    ESP_LOGI(TAG, "Time is not set yet.");
  }

  if (services->enable_ntp)
  {
    ESP_LOGI(TAG, "Connecting to WiFi and getting time over NTP.");
    obtain_time();

    if (ntp_sync)
    {
      time(&now);
      localtime_r(&now, &timeinfo);

      /* Print Local Time esp8266 */
      strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
      ESP_LOGI(TAG, "The NTP date/time is: %s", strftime_buf);

#ifdef USE_RTC
      /* set MCP7940 time in local time */
      datetime.year = 1900 + timeinfo.tm_year - 2000; /* only 2 last dig in STM RTC */
      datetime.month = timeinfo.tm_mon + 1;           /* 0 - Jan */
      datetime.day = timeinfo.tm_mday;
      datetime.weekday = timeinfo.tm_wday;
      datetime.hour = timeinfo.tm_hour;
      datetime.min = timeinfo.tm_min;
      datetime.sec = timeinfo.tm_sec;

      mcp7940_set_datetime(&datetime);
#endif
    }
  }
  else
  {
    ESP_LOGI(TAG, "NTP Disabled. Will use time from MCP7940 rtc.");
  }

  /* update local time */
  time(&now);
  localtime_r(&now, &timeinfo);

  /* Print Local Time */
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The Local date/time is: %s", strftime_buf);

}

void print_time()
{
  time_t now;
  struct tm timeinfo;
  char strftime_buf[64];

  time(&now);
  localtime_r(&now, &timeinfo);

  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The current date/time in %s is: %s", getenv("TZ"), strftime_buf);
}

/* return point to string with current: HH:MM */
void get_time_string(char *time_string)
{
  time_t now;
  struct tm timeinfo;

  if (time_string == NULL)
    return;

  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(time_string, 6, "%R", &timeinfo);
}

void det_time_string_since_boot(char * time_string)
{
  uint64_t since_boot = (uint64_t)esp_timer_get_time();
  uint64_t seconds = since_boot / 1000000;
  uint16_t days = seconds / 86400;
  uint16_t remind_seconds = seconds % 86400;
  uint8_t  hours = remind_seconds / 3600;
  remind_seconds = remind_seconds % 3600;
  uint8_t  minutes = remind_seconds / 60;
  remind_seconds = remind_seconds % 60;

  sniprintf(time_string, 32, "%d days %d:%d:%d", days, hours, minutes, remind_seconds);
}

uint8_t get_ntp_sync_status()
{
  return ntp_sync;
}