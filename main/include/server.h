/***
** Created by Aleksey Volkov on 15.12.2019.
***/

#ifndef ESP32_CC_LED_DRIVER_RTOS_SERVER_H
#define ESP32_CC_LED_DRIVER_RTOS_SERVER_H

#include "esp_http_server.h"

httpd_handle_t start_webserver(void);
void stop_webserver(void);

/* JSON */
char * get_settings_json();
char * get_schedule_json();
char * get_status_json();

#endif //ESP32_CC_LED_DRIVER_RTOS_SERVER_H
