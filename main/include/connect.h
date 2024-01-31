/***
** Created by Aleksey Volkov on 16.07.2020.
***/

#ifndef TFT_DOSER_CONNECT_H
#define TFT_DOSER_CONNECT_H

#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_FAIL_BIT         BIT1

extern EventGroupHandle_t wifi_event_group;

void initialise_wifi(void *arg);
void disable_power_save(void);

#endif //TFT_DOSER_CONNECT_H
