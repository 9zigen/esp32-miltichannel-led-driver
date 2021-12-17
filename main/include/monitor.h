/***
** Created by Aleksey Volkov on 16.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_MONITOR_H
#define HV_CC_LED_DRIVER_RTOS_MONITOR_H

typedef struct {
  char wifi_mode[6];
  char net_address[16];
  char net_mask[16];
  char net_gateway[16];
  char mac[18];
  double mcu_temp;
  uint32_t free_heap;

} system_status_t;

system_status_t* get_system_status(void);
void task_monitor(void *pvParameters);

#endif //HV_CC_LED_DRIVER_RTOS_MONITOR_H
