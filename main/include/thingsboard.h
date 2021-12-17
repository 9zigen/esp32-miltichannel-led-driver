/***
** Created by Aleksey Volkov on 01.01.2020.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_THINGSBOARD_H
#define HV_CC_LED_DRIVER_RTOS_THINGSBOARD_H

#ifdef THINGSBOARD
void task_thingsboard(void *pvParameters);

void init_thingsboard();
void thingsboard_publish_device_status();
#endif

#endif //HV_CC_LED_DRIVER_RTOS_THINGSBOARD_H
