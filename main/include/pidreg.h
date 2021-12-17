//
// Created by Aleksey Volkov on 20.11.2020.
//

#ifndef ESP32_CC_LED_DRIVER_RTOS_PIDREG_H
#define ESP32_CC_LED_DRIVER_RTOS_PIDREG_H

typedef struct {
  double kp;
  double ki;
  double kd;
  double dt;
  double integral;
  double pre_error;
  double min;
  double max;
} pid_reg_t;

pid_reg_t init_pid(double kp, double ki, double kd, double dt, double min, double max);
double calculate(pid_reg_t * pid, double setpoint, double in);

void task_pid_calc(void *pvParameters);

#endif //ESP32_CC_LED_DRIVER_RTOS_PIDREG_H
