/***
** Created by Aleksey Volkov on 22.12.2019.
***/

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>
#include <driver/ledc.h>
#include <string.h>

#include "board.h"
#include "settings.h"
#include "mcp7940.h"
#include "pwm.h"
#include "udp_multicast.h"
#include "light.h"

#define LEDC_FADE_TIME         (3000)

#define RTC_MEM_BRIGHTNESS_OFFSET 7
#define RTC_MEM_CHANNELS_OFFSET 8
#define RTC_MEM_MAGIC 0xAD

static const char *TAG = "LIGHT";

/* current led channel state */
volatile led_schedule_t channel[MAX_LED_CHANNELS] = {0};
volatile double brightness = 0.0;
volatile uint8_t schedule_was_started = false;
volatile uint8_t brake_slow_transition = 0;
volatile uint8_t stop_schedule = 0;

QueueHandle_t xQueueTransition;

/* gamma 2.4 8bit 256steps */
const uint16_t gamma_2_40[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   2,   2,   2,
    3,   3,   4,   4,   5,   5,   6,   6,   7,   8,   9,   9,  10,  11,  12,  13,
    14,  15,  16,  17,  19,  20,  21,  23,  24,  25,  27,  29,  30,  32,  34,  35,
    37,  39,  41,  43,  45,  47,  49,  52,  54,  56,  59,  61,  64,  66,  69,  71,
    74,  77,  80,  83,  86,  89,  92,  95,  98, 102, 105, 109, 112, 116, 119, 123,
    127, 131, 134, 138, 142, 147, 151, 155, 159, 164, 168, 173, 177, 182, 187, 191,
    196, 201, 206, 211, 216, 222, 227, 232, 238, 243, 249, 255, 260, 266, 272, 278,
    284, 290, 296, 303, 309, 316, 322, 329, 335, 342, 349, 356, 363, 370, 377, 384,
    391, 399, 406, 414, 422, 429, 437, 445, 453, 461, 469, 477, 485, 494, 502, 511,
    519, 528, 537, 546, 555, 564, 573, 582, 591, 601, 610, 620, 629, 639, 649, 659,
    669, 679, 689, 699, 710, 720, 731, 741, 752, 763, 774, 785, 796, 807, 818, 829,
    841, 852, 864, 876, 887, 899, 911, 923, 935, 948, 960, 972, 985, 998,1010,1023,
    1036,1049,1062,1075,1089,1102,1115,1129,1143,1156,1170,1184,1198,1212,1227,1241,
    1255,1270,1285,1299,1314,1329,1344,1359,1374,1390,1405,1421,1436,1452,1468,1484,
    1500,1516,1532,1548,1565,1581,1598,1615,1632,1648,1665,1683,1700,1717,1735,1752,
    1770,1788,1805,1823,1841,1860,1878,1896,1915,1933,1952,1971,1990,2009,2028,2047,
};

/* gamma 2.2 8bit 256steps */
const uint16_t gamma_2_20[256] = {
    0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   2,   2,   2,   3,   3,   4,
    5,   5,   6,   7,   8,   8,   9,  10,  11,  12,  13,  15,  16,  17,  18,  20,
    21,  23,  24,  26,  28,  29,  31,  33,  35,  37,  39,  41,  43,  45,  47,  50,
    52,  54,  57,  59,  62,  65,  67,  70,  73,  76,  79,  82,  85,  88,  91,  94,
    98, 101, 105, 108, 112, 115, 119, 123, 127, 131, 135, 139, 143, 147, 151, 155,
    160, 164, 169, 173, 178, 183, 187, 192, 197, 202, 207, 212, 217, 223, 228, 233,
    239, 244, 250, 255, 261, 267, 273, 279, 285, 291, 297, 303, 309, 316, 322, 328,
    335, 342, 348, 355, 362, 369, 376, 383, 390, 397, 404, 412, 419, 427, 434, 442,
    449, 457, 465, 473, 481, 489, 497, 505, 513, 522, 530, 539, 547, 556, 565, 573,
    582, 591, 600, 609, 618, 628, 637, 646, 656, 665, 675, 685, 694, 704, 714, 724,
    734, 744, 755, 765, 775, 786, 796, 807, 817, 828, 839, 850, 861, 872, 883, 894,
    905, 917, 928, 940, 951, 963, 975, 987, 998,1010,1022,1035,1047,1059,1071,1084,
    1096,1109,1122,1135,1147,1160,1173,1186,1199,1213,1226,1239,1253,1266,1280,1294,
    1308,1321,1335,1349,1364,1378,1392,1406,1421,1435,1450,1465,1479,1494,1509,1524,
    1539,1554,1570,1585,1600,1616,1631,1647,1663,1678,1694,1710,1726,1743,1759,1775,
    1791,1808,1824,1841,1858,1875,1891,1908,1925,1943,1960,1977,1994,2012,2029,2047,
};

/* gamma 2.0 13bit 256steps */
const uint16_t gamma_2_00[256] = {
    0,   0,   0,   0,   1,   1,   1,   2,   2,   3,   3,   4,   5,   5,   6,   7,
    8,   9,  10,  11,  13,  14,  15,  17,  18,  20,  21,  23,  25,  26,  28,  30,
    32,  34,  36,  39,  41,  43,  45,  48,  50,  53,  56,  58,  61,  64,  67,  70,
    73,  76,  79,  82,  85,  88,  92,  95,  99, 102, 106, 110, 113, 117, 121, 125,
    129, 133, 137, 141, 146, 150, 154, 159, 163, 168, 172, 177, 182, 187, 192, 196,
    201, 207, 212, 217, 222, 227, 233, 238, 244, 249, 255, 261, 266, 272, 278, 284,
    290, 296, 302, 309, 315, 321, 328, 334, 340, 347, 354, 360, 367, 374, 381, 388,
    395, 402, 409, 416, 424, 431, 438, 446, 453, 461, 469, 476, 484, 492, 500, 508,
    516, 524, 532, 540, 549, 557, 565, 574, 582, 591, 600, 608, 617, 626, 635, 644,
    653, 662, 671, 680, 690, 699, 708, 718, 727, 737, 747, 756, 766, 776, 786, 796,
    806, 816, 826, 836, 847, 857, 867, 878, 888, 899, 910, 921, 931, 942, 953, 964,
    975, 986, 997,1009,1020,1031,1043,1054,1066,1077,1089,1101,1113,1125,1136,1148,
    1160,1173,1185,1197,1209,1222,1234,1247,1259,1272,1285,1297,1310,1323,1336,1349,
    1362,1375,1388,1402,1415,1428,1442,1455,1469,1482,1496,1510,1524,1538,1551,1565,
    1580,1594,1608,1622,1636,1651,1665,1680,1694,1709,1724,1738,1753,1768,1783,1798,
    1813,1828,1844,1859,1874,1890,1905,1921,1936,1952,1968,1983,1999,2015,2031,2047,
};

/* gamma 1.8 13bit 256steps */
const uint16_t gamma_1_80[256] = {
    0,   0,   0,   1,   1,   2,   2,   3,   4,   5,   6,   7,   8,  10,  11,  12,
    14,  16,  17,  19,  21,  23,  25,  27,  29,  31,  34,  36,  38,  41,  43,  46,
    49,  52,  54,  57,  60,  63,  67,  70,  73,  76,  80,  83,  87,  90,  94,  98,
    101, 105, 109, 113, 117, 121, 125, 129, 134, 138, 142, 147, 151, 156, 161, 165,
    170, 175, 180, 185, 190, 195, 200, 205, 210, 215, 221, 226, 232, 237, 243, 248,
    254, 260, 266, 271, 277, 283, 289, 295, 302, 308, 314, 320, 327, 333, 340, 346,
    353, 359, 366, 373, 380, 386, 393, 400, 407, 414, 422, 429, 436, 443, 451, 458,
    466, 473, 481, 488, 496, 504, 511, 519, 527, 535, 543, 551, 559, 567, 575, 584,
    592, 600, 609, 617, 626, 634, 643, 652, 660, 669, 678, 687, 696, 705, 714, 723,
    732, 741, 750, 759, 769, 778, 788, 797, 807, 816, 826, 835, 845, 855, 865, 875,
    885, 895, 905, 915, 925, 935, 945, 956, 966, 976, 987, 997,1008,1018,1029,1039,
    1050,1061,1072,1083,1094,1105,1116,1127,1138,1149,1160,1171,1183,1194,1205,1217,
    1228,1240,1251,1263,1275,1286,1298,1310,1322,1334,1346,1358,1370,1382,1394,1406,
    1419,1431,1443,1456,1468,1481,1493,1506,1518,1531,1544,1556,1569,1582,1595,1608,
    1621,1634,1647,1660,1674,1687,1700,1713,1727,1740,1754,1767,1781,1794,1808,1822,
    1835,1849,1863,1877,1891,1905,1919,1933,1947,1961,1975,1990,2004,2018,2033,2047,
};

static int16_t minutes_left(uint8_t hour, uint8_t minute, uint8_t schedule_hour, uint8_t schedule_minute)
{
  return (schedule_hour - hour) * 60 + schedule_minute - minute;
}

static int32_t seconds_left(uint8_t hour, uint8_t minute, uint8_t second, uint8_t schedule_hour, uint8_t schedule_minute)
{
  return (schedule_hour - hour) * 3600 + (schedule_minute - minute) * 60 + (0 - second);
}

uint8_t get_brightness()
{
  return brightness;
}

uint8_t get_channel_duty(uint8_t id)
{
  if (id >= MAX_LED_CHANNELS)
    return 0;

  return channel[id].current_duty;
}

uint8_t get_channel_state(uint8_t id)
{
  if (id >= MAX_LED_CHANNELS)
    return 0;

  if (channel[id].current_duty > 0)
  {
    return 1;
  }
  return 0;
}

uint8_t light_is_on()
{
  uint8_t led_active = 0;
  for (uint id = 0; id < MAX_LED_CHANNELS; ++id) {
    if (channel[id].current_duty)
    {
      led_active = 1;
    }
  }
  return led_active;
}

void set_channel_state(uint8_t id, uint8_t state)
{
  if (id >= MAX_LED_CHANNELS)
    return;

  if (state == 1)
  {
    set_channel_duty(id, 255, 0);
  } else {
    set_channel_duty(id, 0, 0);
  }
}

/* set selected channel duty */
void set_channel_duty(uint8_t id, uint8_t duty, uint8_t not_sync)
{
  if (id >= MAX_LED_CHANNELS)
    return;

  x_light_message_t txMessage;
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* prepare new queue message */
    txMessage.target_duty[i] = channel[i].current_duty;
    if (i == id)
    {
      /* max duty in % */
      led_t * led = get_leds(i);
      channel[i].power_factor = led->duty_max / 255.0;

      txMessage.target_duty[i] = duty;
    }
  }

  /* transition duration LEDC_FADE_TIME in ms. */
  txMessage.target_brightness = brightness;
  txMessage.transition_mode = FAST;
  txMessage.fade_time = LEDC_FADE_TIME;
  txMessage.not_sync = not_sync;

  /* send new queue */
  brake_slow_transition = 1;
  xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
}

void set_brightness(uint8_t target_brightness, uint8_t not_sync)
{

  x_light_message_t txMessage;
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* prepare new queue message */
    txMessage.target_duty[i] = channel[i].current_duty;
  }

  /* transition duration LEDC_FADE_TIME in ms. */
  txMessage.target_brightness = target_brightness;
  txMessage.transition_mode = FAST;
  txMessage.fade_time = LEDC_FADE_TIME;
  txMessage.not_sync = not_sync;

  /* send new queue */
  brake_slow_transition = 1;
  stop_schedule = 1;
  xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
}

void set_light(const double * target_duty, double target_brightness, uint8_t mode, uint8_t not_sync)
{
  x_light_message_t txMessage;
  for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
  {
    /* max duty in % */
    led_t * led = get_leds(i);
    channel[i].power_factor = led->duty_max / 255.0;

    /* prepare new queue message */
    txMessage.target_duty[i] = target_duty[i];
  }

  /* transition duration LEDC_FADE_TIME in ms. */
  txMessage.target_brightness = target_brightness;
  txMessage.transition_mode = mode;
  txMessage.fade_time = LEDC_FADE_TIME;
  txMessage.not_sync = not_sync;

  if (mode == FAST) {
    brake_slow_transition = 1;
    stop_schedule = 1;
  }

  /* send new queue */
  xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
}

static void backup_channels()
{
  esp_err_t err = ESP_OK;
  led_schedule_rtc_mem_t mem;

  memset(&mem, 0, sizeof(led_schedule_rtc_mem_t));

  mem.magic = RTC_MEM_MAGIC;
  mem.brightness = (uint8_t)brightness;
  for (int i = 0; i < MAX_LED_CHANNELS; i++) {
    mem.duty[i] = (uint8_t)channel[i].current_duty;
  }

  err = mcp7940_write_ram(RTC_MEM_CHANNELS_OFFSET, (uint8_t *)&mem, sizeof(led_schedule_rtc_mem_t));

  if (err == ESP_OK) {
    ESP_LOGD(TAG, "rtc backup ok!");
  }
}

static void restore_channels()
{
  led_schedule_rtc_mem_t mem;
  double duty[MAX_LED_CHANNELS];

  mcp7940_read_ram(RTC_MEM_CHANNELS_OFFSET, (uint8_t *)&mem, sizeof(led_schedule_rtc_mem_t));

  /* restore light */
  if (mem.magic == RTC_MEM_MAGIC) {
    for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
      duty[i] = mem.duty[i];
    }
    set_light(duty, (double)mem.brightness, FAST, 0);
  }
}

/**
 *
 * @param x0 left side time point
 * @param y0 left side duty
 * @param x1 right side time point
 * @param y1 right side duty
 * @param xp current time point
 * @return current duty
 */
static double calc_interpolation(int x0, int y0, int x1, int y1, int xp)
{
  return y0 + ((double)(y1-y0)/(x1-x0)) * (xp - x0);
}

/* main light control task */
void task_light(void *pvParameter)
{
  time_t now;
  struct tm timeinfo;
  x_light_message_t txMessage;

  /* light transition queue */
  xQueueTransition = xQueueCreate( 10, sizeof( x_light_message_t ) );

  schedule_config_t * schedule_config;

  time(&now);
  localtime_r(&now, &timeinfo);

  /* demo firmware */
//  if (timeinfo.tm_year != (2021 - 1900) && timeinfo.tm_mon != 5) {
//    ESP_LOGI(TAG, "END DEMO TIME");
//    vTaskDelete(NULL);
//  }

  /* check if first boot */
  bool first_boot = true;
  schedule_config = get_schedule_config();
  if (schedule_config->mode != SIMPLE) {
    for (int i = 0; i < MAX_SCHEDULE; ++i) {
      schedule_t * schedule = get_schedule(i);
      if (schedule->active) { first_boot = false; }
    }
  } else {
    first_boot = false;
  }

  /* power on 30% */
  if (first_boot) {
    double duty[MAX_LED_CHANNELS];
    for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
      duty[i] = 255.0;
    }
    set_light(duty, 30.0, FAST, 0);
    ESP_LOGW(TAG, "First boot: Brightness 0 -> 30");
  } else {
    /* load current pwm values from rtc */
    restore_channels();
  }

  while (1)
  {
    /* get current local time */
    time(&now);
    localtime_r(&now, &timeinfo);

    /* simple mode sunrise/sunset */
    schedule_config = get_schedule_config();
    if (schedule_config->mode == SIMPLE)
    {
      int sunrise_munutes_left = minutes_left(timeinfo.tm_hour, timeinfo.tm_min, schedule_config->sunrise_hour, schedule_config->sunrise_minute);
      int sunset_munutes_left = minutes_left(timeinfo.tm_hour, timeinfo.tm_min, schedule_config->sunset_hour, schedule_config->sunset_minute);

      /* Sunrise */
      if (sunrise_munutes_left == 0 && !schedule_was_started) {
        for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
        {
          /* max duty in % */
          led_t * led = get_leds(i);
          channel[i].power_factor = led->duty_max / 255.0;

          /* prepare new queue message */
          txMessage.target_duty[i] = schedule_config->duty[i];
        }
      }

      /* Sunset */
      else if (sunset_munutes_left == 0 && !schedule_was_started) {
        for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
        {
          /* prepare new queue message */
          txMessage.target_duty[i] = 0;
        }
      }

      /* Run */
      if ((sunrise_munutes_left == 0 || sunset_munutes_left == 0) && !schedule_was_started) {
        schedule_was_started = 1;
        /* transition duration LEDC_FADE_TIME in ms. */
        txMessage.target_brightness = schedule_config->brightness;
        txMessage.transition_mode = SLOW;
        txMessage.fade_time = LEDC_FADE_TIME;
        txMessage.not_sync = 0;

        /* send new queue */
        xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
      }

    } else {
      /* advanced schedule ---> */
      /* left side */
      int nearest_left_side_sec = 0;
      int min_left_side_distance_idx = -1;

      /* right side */
      int nearest_right_side_sec = 0;
      int min_right_side_distance_idx = -1;

      for (int j = 0; j < MAX_SCHEDULE; ++j) {
        schedule_t *_schedule = get_schedule(j);

        /* only enable schedule process */
        if (_schedule->active) {
          int sec_left = seconds_left(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, _schedule->time_hour, _schedule->time_minute);

          /* start light transition */
          if (sec_left == 0) {
            /* reset flag, previously setted by manual control */
            stop_schedule = 0;

            for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
            {
              /* max duty in % */
              led_t * led = get_leds(i);
              channel[i].power_factor = led->duty_max / 255.0;

              /* prepare new queue message */
              txMessage.target_duty[i] = _schedule->duty[i];
            }

            /* transition duration LEDC_FADE_TIME in ms. */
            txMessage.target_brightness = _schedule->brightness;
            txMessage.transition_mode = FAST;
            txMessage.fade_time = 600;
            txMessage.not_sync = 0;

            /* send new queue */
            xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
          }

          /* left side */
          if (sec_left < 0) {
            if (nearest_left_side_sec == 0 || sec_left > nearest_left_side_sec) {
              nearest_left_side_sec = sec_left;
              min_left_side_distance_idx = j;
            }
          }
          ESP_LOGD(TAG, "LEFT SIDE left: %d, idx %d", sec_left, min_left_side_distance_idx);

          /* right side */
          if (sec_left > 0) {
            if (nearest_right_side_sec == 0 || sec_left < nearest_right_side_sec) {
              nearest_right_side_sec = sec_left;
              min_right_side_distance_idx = j;
            }
          }
          ESP_LOGD(TAG, "RIGHT SIDE left: %d, idx %d", sec_left, min_right_side_distance_idx);
        }
      }
      ESP_LOGD(TAG, "LEFT SIDE SELECTED idx %d", min_left_side_distance_idx);
      ESP_LOGD(TAG, "RIGHT SIDE SELECTED idx %d", min_right_side_distance_idx);

      if (!stop_schedule) {
        /* calc interpolation
      ** yp = y0 + ((y1-y0)/(x1-x0)) * (xp - x0); */
        if (min_left_side_distance_idx > -1 && min_right_side_distance_idx > -1) {
          /* get nearest schedule point */
          if (min_left_side_distance_idx < MAX_SCHEDULE && min_right_side_distance_idx < MAX_SCHEDULE) {
            schedule_t *ls_schedule = get_schedule(min_left_side_distance_idx);
            schedule_t *rs_schedule = get_schedule(min_right_side_distance_idx);

            int ls_point = ls_schedule->time_hour * 3600 + ls_schedule->time_minute * 60;
            int rs_point = rs_schedule->time_hour * 3600 + rs_schedule->time_minute * 60;
            int current_point = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
            ESP_LOGD(TAG, "time points %d --> %d ---> %d", ls_point, current_point, rs_point);

            for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i)
            {
              /* max duty in % */
              led_t * led = get_leds(i);
              channel[i].power_factor = led->duty_max / 255.0;

              /* prepare new queue message */
              txMessage.target_duty[i] = calc_interpolation(ls_point, ls_schedule->duty[i], rs_point, rs_schedule->duty[i], current_point);
            }

            /* transition duration LEDC_FADE_TIME in ms. */
            txMessage.target_brightness = calc_interpolation(ls_point, ls_schedule->brightness, rs_point, rs_schedule->brightness, current_point);
            txMessage.transition_mode = FAST;
            txMessage.fade_time = 600;
            txMessage.not_sync = 0;

            /* send new queue */
            xQueueSendToBack(xQueueTransition, &txMessage, 100/portTICK_RATE_MS);
          }
        }
      }
    }

    /* 900 msec. delay */
    vTaskDelay(900 / portTICK_RATE_MS);
  }
}

/* Return 0 - 255 gamma corrected value */
uint16_t gamma_correction(uint8_t value, uint8_t gamma) {
  switch (gamma) {
    case GAMMA_1_80:
      return gamma_1_80[value];
      break;
    case GAMMA_2_00:
      return gamma_2_00[value];
      break;
    case GAMMA_2_20:
      return gamma_2_20[value];
      break;
    case GAMMA_2_40:
      return gamma_2_40[value];
      break;
    default:
      return value;
  }
}

/* Update channel state, color transition process schedule end manual changed */
void task_light_transition(void *pvParameter)
{
  double steps_left;
  double difference;
  double target_real_duty;
  x_light_message_t rxMessage;

  if( xQueueTransition != 0 )
  {
    for(;;)
    {
      if (xQueueReceive(xQueueTransition, &rxMessage, portMAX_DELAY))
      {
        /* new transition received, process all channels together */
        schedule_config_t * schedule_config = get_schedule_config();

        ESP_LOGD(TAG, "new queue --------- ");
        ESP_LOGD(TAG, "transition_mode: %d", rxMessage.transition_mode);
        ESP_LOGD(TAG, "brightness: %f -> %f", brightness, rxMessage.target_brightness);
        ESP_LOGD(TAG, "gamma: %d", schedule_config->gamma);

        /* transition steps counter */
        steps_left = 0;
        double max_diff = 0;

        /* sync lights */
        if (!rxMessage.not_sync) {
          double sync_duty[MAX_LED_CHANNELS];
          for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i) {
            sync_duty[i] = rxMessage.target_duty[i];
          }
          udp_set_light(sync_duty, rxMessage.target_brightness, rxMessage.transition_mode);
        }

        for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i) {
          /* calc target duty on end of transition */
          if (schedule_config->gamma == GAMMA_1_00) {
            target_real_duty = rxMessage.target_duty[i] / 255.0 * DUTY_STEPS;
          } else {
            target_real_duty = gamma_correction(rxMessage.target_duty[i], schedule_config->gamma);
          }

          /* brightness adjust */
          target_real_duty = target_real_duty * rxMessage.target_brightness / MAX_BRIGHTNESS;
          difference = fabs(target_real_duty - channel[i].real_duty);

          ESP_LOGD(TAG, "\t CH: %d Duty: %f Real: %f -> %f (%f)",
                   i, channel[i].current_duty, channel[i].real_duty, target_real_duty, difference);

          /* save maximum available duty delta */
          if (difference > max_diff) {
            max_diff = difference;
          }
        }

        /* nothing to change skip all */
        if (max_diff == 0) {
          steps_left = 0;
        }

        /* calc right step count; 600: 10 min. transition */
        else if (max_diff > 0 && max_diff < 1800) {
          steps_left = (uint32_t) max_diff;
        }
        else {
          steps_left = 1800;
        }

        ESP_LOGD(TAG, "\t Calculated steps count: %f", steps_left);

        /* RUN schedule transition */
        if (rxMessage.transition_mode == SLOW) {
          while (steps_left > 0) {
            /* check brake flag */
            if (brake_slow_transition) {
              steps_left = 0;
            } else {
              for (uint8_t i = 0; i < MAX_LED_CHANNELS; ++i) {

                /* Calc New Current Duty */
                if (channel[i].current_duty != rxMessage.target_duty[i]) {
                  difference = (rxMessage.target_duty[i] - channel[i].current_duty) / steps_left;
                  channel[i].current_duty = fabs(channel[i].current_duty + difference);
                }

                /* Calc New Brightness */
                if (brightness != rxMessage.target_brightness) {
                  difference = (rxMessage.target_brightness - brightness) / steps_left;
                  brightness = fabs(brightness + difference);
                  ESP_LOGD(TAG, "\t CH: %d Brightness: %f -> %f (%f)", i, brightness, rxMessage.target_brightness, difference);
                }

                /* Calc New Real Duty, brightness & gamma compensated */
                if (schedule_config->gamma == GAMMA_1_00) {
                  target_real_duty = channel[i].current_duty / 255.0 * DUTY_STEPS;
                } else {
                  target_real_duty = gamma_correction(channel[i].current_duty, schedule_config->gamma);
                }

                /* brightness adjust */
                target_real_duty = target_real_duty * brightness / MAX_BRIGHTNESS;

                /* Brightness / Duty changed */
                if (channel[i].real_duty != target_real_duty) {
                  difference = fabs(target_real_duty - channel[i].real_duty);

                  ESP_LOGD(TAG, "\t CH: %d LEDC %f -> %f (%f) steps left: %f",
                           i, channel[i].real_duty, target_real_duty, difference, steps_left);

                  channel[i].real_duty = target_real_duty;

                  /* power factor */
                  double real_duty = channel[i].real_duty * channel[i].power_factor;

                  if (difference > 2 && difference <= 1023) {
                    ledc_fade(i, (uint32_t)real_duty, (uint32_t) difference);
                  } else if (difference > 2) {
                    ledc_fade_ms(i, (uint32_t)real_duty, 1000);
                  } else {
                    ledc_set(i, (uint32_t)real_duty);
                  }
                }
              }

              steps_left--;
              vTaskDelay(1000 / portTICK_RATE_MS);
            }
          }
        }

        /* check brake flag */
        if (rxMessage.transition_mode == SLOW && brake_slow_transition) {
          ESP_LOGD(TAG, "queue braked!");
        } else {
          /* end transition: step 0 */
          for (int j = 0; j < MAX_LED_CHANNELS; ++j) {

            /* calc target duty on end of transition */
            if (schedule_config->gamma == GAMMA_1_00) {
              target_real_duty = rxMessage.target_duty[j] / 255.0 * DUTY_STEPS;
            } else {
              target_real_duty = gamma_correction(rxMessage.target_duty[j], schedule_config->gamma);
            }

            /* brightness adjust */
            target_real_duty = target_real_duty * rxMessage.target_brightness / MAX_BRIGHTNESS;
            difference = fabs(target_real_duty - channel[j].real_duty);

            /* update current channel values */
            channel[j].current_duty = rxMessage.target_duty[j];
            channel[j].real_duty = target_real_duty;

            /* power factor */
            double real_duty = channel[j].real_duty * channel[j].power_factor;

            ESP_LOGD(TAG, "\t power factor: %f LEDC %f -> %f",
                     channel[j].power_factor, target_real_duty, real_duty);

            /* fast transition by UI or MQTT */
            if (rxMessage.transition_mode == FAST && difference > 10) {
              ledc_fade_ms(j, (uint32_t)real_duty, 3000);
            } else {
              ledc_set(j, (uint32_t)real_duty);
            }
          }

          /* update current brightness */
          brightness = rxMessage.target_brightness;

          /* reset schedule start flag */
          schedule_was_started = 0;

          ESP_LOGD(TAG, "queue done!");
        }

        /* Backup PWM in RTC */
        backup_channels();

        /* reset brake flag */
        brake_slow_transition = 0;
      }
    }
  }
}