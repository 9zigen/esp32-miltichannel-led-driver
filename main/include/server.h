/***
** Created by Aleksey Volkov on 15.12.2019.
***/

#ifndef RTOS_TEMPLATE_SERVER_H
#define RTOS_TEMPLATE_SERVER_H

#include "esp_http_server.h"

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

/* JSON */
char * get_settings_json();
char * get_schedule_json();
char * get_status_json();
#endif //RTOS_TEMPLATE_SERVER_H
