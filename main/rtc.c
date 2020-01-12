/***
** Created by Aleksey Volkov on 19.12.2019.
***/

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/event_groups.h>
#include <stmdriver.h>
#include <esp_timer.h>

#include "esp_log.h"
#include "esp_system.h"
#include "lwip/apps/sntp.h"

#include "main.h"
#include "settings.h"
#include "rtc.h"

static const char *TAG = "RTC";

uint8_t ntp_sync = 0;

static void initialize_sntp(services_t * config)
{
  ESP_LOGI(TAG, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, config->ntp_server);
  sntp_init();
}

static void obtain_time(void)
{
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

  services_t * services = get_service_config();

  if (services->enable_ntp && strlen(services->ntp_server) > 5)
  {
    initialize_sntp(services);
  }

  // wait for time to be set
  time_t now = 0;
  struct tm timeinfo = { 0 };
  int retry = 0;
  const int retry_count = 10;

  while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count) {
    ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    time(&now);
    localtime_r(&now, &timeinfo);
  }

  if (retry < retry_count) {
    ntp_sync = 1;
  }
}

void init_clock()
{
  time_t now;
  struct tm timeinfo;
  char strftime_buf[64];

  /* load config */
  services_t * services = get_service_config();

  /* Get time from stm32 rtc */
  stm_datetime_t datetime;
  get_stm_rtc(&datetime);

  timeinfo.tm_year = datetime.year + 2000 - 1900;  /* stm return only last two dig of year */
  timeinfo.tm_mon  = datetime.month - 1;
  timeinfo.tm_mday = datetime.day;
  timeinfo.tm_wday = datetime.weekday;
  timeinfo.tm_hour = datetime.hour;
  timeinfo.tm_min  = datetime.minute;
  timeinfo.tm_sec  = datetime.second;

  time_t stm_time = mktime(&timeinfo);
  struct timeval stm_now = { .tv_sec = stm_time };

  /* Set time from STM RTC */
  settimeofday(&stm_now, NULL);

  /* Print time from STM RTC */
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The current date/time in STM RTC is: %s", strftime_buf);

  // Is time set? If not, tm_year will be (1970 - 1900).
  if (timeinfo.tm_year < (2020 - 1900)) {
    ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
    obtain_time();

    time(&now);
    localtime_r(&now, &timeinfo);

    /* Print Local Time esp8266 */
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The NTP date/time is: %s", strftime_buf);

    /* set STM time in local time */
    datetime.year = 1900 + timeinfo.tm_year - 2000; /* only 2 last dig in STM RTC */
    datetime.month = timeinfo.tm_mon + 1;           /* 0 - Jan */
    datetime.day = timeinfo.tm_mday;
    datetime.weekday = timeinfo.tm_wday;
    datetime.hour = timeinfo.tm_hour;
    datetime.minute = timeinfo.tm_min;
    datetime.second = timeinfo.tm_sec;

    set_stm_rtc(&datetime);
  }

  /* Set timezone from config */
  char tz_buff[32];
  if (services->utc_offset > 0) {
    snprintf(tz_buff, 32, "UTC-%d", services->utc_offset + ((uint8_t)services->ntp_dst));    //GMT+2
  } else {
    snprintf(tz_buff, 32, "UTC+%d", services->utc_offset + ((uint8_t)services->ntp_dst));    //UTC-2
  }
  setenv("TZ", tz_buff, 1);
  ESP_LOGI(TAG, "new TZ is: %s", getenv("TZ"));
  tzset();

  /* update local time */
  time(&now);
  localtime_r(&now, &timeinfo);

  /* Print Local Time esp8266 */
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The Local esp8266 date/time is: %s", strftime_buf);

}

/* Convert from epoch to human-readable date */
int convert_time_from_epoch(void)
{
  time_t     now;
  struct tm  ts;
  char       buf[80];

  // Get current time
  time(&now);

  // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
  ts = *localtime(&now);
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("%s\n", buf);
  return 0;
}

/* Convert from human-readable date to epoch */
void convert_time_to_epoch(void)
{
  struct tm t;
  time_t t_of_day;

  t.tm_year = 2019-1900;  // Year - 1900
  t.tm_mon = 7;           // Month, where 0 = jan
  t.tm_mday = 8;          // Day of the month
  t.tm_hour = 16;
  t.tm_min = 11;
  t.tm_sec = 42;
  t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
  t_of_day = mktime(&t);

  printf("seconds since the Epoch: %ld\n", (long) t_of_day);
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