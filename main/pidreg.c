//
// Created by Aleksey Volkov on 20.11.2020.
//
#include <math.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_system.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "board.h"
#include "settings.h"
#include "pwm.h"
#include "adc.h"
#include "monitor.h"
#include "fanspeed.h"
#include "light.h"
#include "pidreg.h"

static const char *TAG = "PID";

pid_reg_t init_pid(double kp, double ki, double kd, double dt, double min, double max)
{
  pid_reg_t pid;
  pid.kp = kp;
  pid.ki = ki;
  pid.kd = kd;
  pid.dt = dt;
  pid.integral = 0;
  pid.pre_error = 0;
  pid.min = min;
  pid.max = max;

  return pid;
}

double calculate(pid_reg_t * pid, double setpoint, double in)
{
  // Calculate error
  double error = setpoint - in;

  // Proportional term
  double Pout = pid->kp * error;

  // Integral term
  pid->integral += error * pid->dt;
  double Iout = pid->ki * pid->integral;

  // Derivative term
  double derivative = (error - pid->pre_error) / pid->dt;
  double Dout = pid->kd * derivative;

  // Calculate total output
  double output = Pout + Iout + Dout;

  // Restrict to max/min
  if( output > pid->max )
    output = pid->max;
  else if( output < pid->min )
    output = pid->min;

  // Save error to previous error
  pid->pre_error = error;

  return output;
}

void task_pid_calc(void *pvParameters)
{
  system_status_t * status = get_system_status();

  cooling_t * cooling = get_cooling();
  TickType_t xLastAdjTime = 0;
  double temp_delta = cooling->max_temp - cooling->target_temp;
  pid_reg_t pid = init_pid(cooling->pid_kp, cooling->pid_ki, cooling->pid_kd, 0.2, 0 - temp_delta, 0);

  /* check fan */
  ledc_fan_set(2047);
  vTaskDelay(3000/portTICK_RATE_MS);
  ESP_LOGE(TAG, "Fan speed %d", get_fan_rpm());
  ledc_fan_set(0);

  while (1) {

    if (!cooling->installed) {
      vTaskDelete(NULL);
    }

#if defined(USE_FAN_ALWAYS)
    uint8_t brightness = get_brightness();
    if (brightness > 10) {
      ledc_fan_set(2047);
    } else {
      ledc_fan_set(0);
    }
    vTaskDelay(1000 / portTICK_RATE_MS);
#else
    double temp = read_ntc_temperature();

    /* ntc not installed */
    if (temp == -273) {
      temp = status->mcu_temp;
    }

    double out = calculate(&pid, (double)cooling->target_temp, temp);

    if ((xTaskGetTickCount() - xLastAdjTime) > (5000/portTICK_RATE_MS)) {
      xLastAdjTime = xTaskGetTickCount();

      if (light_is_on())
      {
        if (temp >= cooling->start_temp && temp < cooling->max_temp) {
          double duty = fabs(out) * 2047.0 / temp_delta;
          ledc_fan_set((uint32_t)duty);
        } else if (temp >= cooling->max_temp) {
          ledc_fan_set(2047);
        } else if (temp < cooling->start_temp) {
          ledc_fan_set(0);
        }
      } else {
        ledc_fan_set(0);
      }
    }

    vTaskDelay(250 / portTICK_RATE_MS);
#endif
  }
}