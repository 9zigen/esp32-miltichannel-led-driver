//
// Created by Aleksey Volkov on 13.04.2021.
//

#ifndef ESP32_CC_LED_DRIVER_RTOS_UDP_MULTICAST_H
#define ESP32_CC_LED_DRIVER_RTOS_UDP_MULTICAST_H

#include "light.h"

typedef struct {

} rx_packet_sync_t;

void udp_set_light(const double *target_duty, double target_brightness);
void mcast_udp_task(void *pvParameters);

#endif //ESP32_CC_LED_DRIVER_RTOS_UDP_MULTICAST_H
