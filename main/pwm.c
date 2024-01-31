//
// Created by Aleksey Volkov on 19.11.2020.
//

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_err.h>
#include <driver/ledc.h>

#include "board.h"
#include "pwm.h"

#define LEDC_HS_TIMER          LEDC_TIMER_0
#ifdef USE_FAN_PWM
#define LEDC_HS_TIMER_FAN      LEDC_TIMER_1
#endif

#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE

/* Hardware specific */
#ifdef PICO_D4_5CH_LED_DRIVER_AIO
#define LEDC_CH_NUM         (5)
#define LEDC_CH0_GPIO       (18) /* channel 1 */
#define LEDC_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_CH1_GPIO       (23) /* channel 2 */
#define LEDC_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_CH2_GPIO       (19) /* channel 3 */
#define LEDC_CH2_CHANNEL    LEDC_CHANNEL_2
#define LEDC_CH3_GPIO       (22) /* channel 4 */
#define LEDC_CH3_CHANNEL    LEDC_CHANNEL_3
#define LEDC_CH4_GPIO       (21) /* channel 5 */
#define LEDC_CH4_CHANNEL    LEDC_CHANNEL_4
#define LEDC_FAN_GPIO       (4) /* Fan channel */
#define LEDC_FAN_CHANNEL    LEDC_CHANNEL_5
#define LEDC_HS_TIMER_FAN   LEDC_TIMER_1
#elif defined(CUSTOM_7CH_LED_DRIVER_AIO)
#define LEDC_CH_NUM         (7)
#define LEDC_CH0_GPIO       (21) /* channel 1 */
#define LEDC_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_CH1_GPIO       (22) /* channel 2 */
#define LEDC_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_CH2_GPIO       (19) /* channel 3 */
#define LEDC_CH2_CHANNEL    LEDC_CHANNEL_2
#define LEDC_CH3_GPIO       (23) /* channel 4 */
#define LEDC_CH3_CHANNEL    LEDC_CHANNEL_3
#define LEDC_CH4_GPIO       (18) /* channel 5 */
#define LEDC_CH4_CHANNEL    LEDC_CHANNEL_4
#define LEDC_CH5_GPIO       (25) /* channel 6 */
#define LEDC_CH5_CHANNEL    LEDC_CHANNEL_5
#define LEDC_CH6_GPIO       (33) /* channel 7 */
#define LEDC_CH6_CHANNEL    LEDC_CHANNEL_6
#endif

static const char *TAG = "PWM";

/*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */
ledc_channel_config_t ledc_channel[LEDC_CH_NUM] = {
    {
        .channel    = LEDC_CH0_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_CH0_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
    {
        .channel    = LEDC_CH1_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_CH1_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
    {
        .channel    = LEDC_CH2_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_CH2_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
#if LEDC_CH_NUM >= 4
    {
        .channel    = LEDC_CH3_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_CH3_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
#endif
#if LEDC_CH_NUM >= 5
    {
        .channel    = LEDC_CH4_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_CH4_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
#endif
#if LEDC_CH_NUM >= 6
    {
        .channel    = LEDC_CH5_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_CH5_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_num  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
#endif
#if LEDC_CH_NUM >= 7
    {
        .channel    = LEDC_CH6_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_CH6_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_num  = LEDC_HS_TIMER,
        .flags.output_invert = 0
    },
#else
//    {
//        .channel    = LEDC_CH5_CHANNEL,
//        .duty       = 0,
//        .gpio_num   = LEDC_CH5_GPIO,
//        .speed_mode = LEDC_HS_MODE,
//        .hpoint     = 0,
//        .timer_num  = LEDC_HS_TIMER
//    },
//    {
//        .channel    = LEDC_HS_CH6_CHANNEL,
//        .duty       = 0,
//        .gpio_num   = LEDC_HS_CH6_GPIO,
//        .speed_mode = LEDC_HS_MODE,
//        .hpoint     = 0,
//        .timer_num  = LEDC_HS_TIMER
//    },
//    {
//        .channel    = LEDC_HS_CH7_CHANNEL,
//        .duty       = 0,
//        .gpio_num   = LEDC_HS_CH7_GPIO,
//        .speed_mode = LEDC_HS_MODE,
//        .hpoint     = 0,
//        .timer_num  = LEDC_HS_TIMER
//    }
#endif

};

#ifdef USE_FAN_PWM
ledc_channel_config_t ledc_channel_fan = {
    .channel    = LEDC_FAN_CHANNEL,
    .duty       = 0,
    .gpio_num   = LEDC_FAN_GPIO,
    .speed_mode = LEDC_HS_MODE,
    .hpoint     = 0,
    .timer_sel  = LEDC_HS_TIMER_FAN,
    .flags.output_invert = 0
};
#endif

void init_ledc()
{
  uint8_t ch;

  /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
  ledc_timer_config_t ledc_timer = {
      .duty_resolution = LEDC_TIMER_11_BIT, // resolution of PWM duty
      .freq_hz = 20000,                     // frequency of PWM signal
      .speed_mode = LEDC_HS_MODE,           // timer mode
      .timer_num = LEDC_HS_TIMER,           // timer index
      .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
  };

  // Set configuration of timer0 for high speed channels
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
  // Set LED Controller with previously prepared configuration
  for (ch = 0; ch < LEDC_CH_NUM; ch++) {
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[ch]));
//    ESP_ERROR_CHECK(ledc_set_pin(ledc_channel[ch].gpio_num, ledc_channel[ch].speed_mode, ledc_channel[ch].timer_sel));
  }

#ifdef USE_FAN_PWM
  ledc_timer_config_t ledc_timer_fan = {
      .duty_resolution = LEDC_TIMER_11_BIT, // resolution of PWM duty
      .freq_hz = 20000,                     // frequency of PWM signal
      .speed_mode = LEDC_HS_MODE,           // timer mode
      .timer_num = LEDC_HS_TIMER_FAN,       // timer index
      .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
  };

  /* Last channel FAN */
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_fan));
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_fan));
//  ESP_ERROR_CHECK(ledc_set_pin(ledc_channel_fan.gpio_num, ledc_timer_fan.speed_mode, ledc_timer_fan.timer_num));
#endif

  // Initialize fade service.
  ESP_ERROR_CHECK(ledc_fade_func_install(0));

  /* off all led */
  for (uint8_t i = 0; i < LEDC_CH_NUM; ++i) {
    ledc_set(i, 0);
  }
}

void ledc_fade(uint8_t channel, uint32_t duty, uint32_t steps)
{
  ESP_LOGD(TAG, "ledc_fade duty: %lu steps: %lu", duty, steps);

  ledc_set_fade_with_step(ledc_channel[channel].speed_mode,
                          ledc_channel[channel].channel,
                          duty,
                          1,
                          steps);
  ledc_fade_start(ledc_channel[channel].speed_mode, ledc_channel[channel].channel, LEDC_FADE_NO_WAIT);
}

void ledc_fade_ms(uint8_t channel, uint32_t duty, int max_time_ms)
{
  ESP_LOGD(TAG, "ledc_fade_ms duty: %lu time: %d", duty, max_time_ms);

  ledc_set_fade_with_time(ledc_channel[channel].speed_mode,
                          ledc_channel[channel].channel,
                          duty,
                          max_time_ms);
  ledc_fade_start(ledc_channel[channel].speed_mode, ledc_channel[channel].channel, LEDC_FADE_NO_WAIT);
}

void ledc_set(uint8_t channel, uint32_t duty)
{
  int hpoint = ledc_get_hpoint(ledc_channel[channel].speed_mode, ledc_channel[channel].channel);
  ledc_set_duty_and_update(ledc_channel[channel].speed_mode, ledc_channel[channel].channel, duty, hpoint);
//  ESP_ERROR_CHECK(ledc_set_duty(ledc_channel[channel].speed_mode, ledc_channel[channel].channel, duty));
//  ESP_ERROR_CHECK(ledc_update_duty(ledc_channel[channel].speed_mode, ledc_channel[channel].channel));
}

#ifdef USE_FAN_PWM
void ledc_fan_set(uint32_t duty)
{
//  ESP_ERROR_CHECK(ledc_set_duty(ledc_channel_fan.speed_mode, ledc_channel_fan.channel, duty));
//  ESP_ERROR_CHECK(ledc_update_duty(ledc_channel_fan.speed_mode, ledc_channel_fan.channel));
  if (ledc_fan_get() != duty) {
    ledc_set_fade_with_time(ledc_channel_fan.speed_mode,
                            ledc_channel_fan.channel,
                            duty,
                            500);
    ledc_fade_start(ledc_channel_fan.speed_mode, ledc_channel_fan.channel, LEDC_FADE_WAIT_DONE);
  }
}
uint32_t ledc_fan_get()
{
  return ledc_get_duty(ledc_channel_fan.speed_mode, ledc_channel_fan.channel);
}
#endif