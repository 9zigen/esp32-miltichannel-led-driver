/***
** Created by Aleksey Volkov on 21.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_MAIN_H
#define HV_CC_LED_DRIVER_RTOS_MAIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

extern EventGroupHandle_t wifi_event_group;
extern const int CONNECTED_BIT;
extern const int AP_CONNECTED_BIT;

esp_err_t upgrade_firmware();

#endif //HV_CC_LED_DRIVER_RTOS_MAIN_H
