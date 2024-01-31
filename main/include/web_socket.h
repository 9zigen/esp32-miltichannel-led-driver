//
// Created by Aleksey Volkov on 04.04.2022.
//

#ifndef ESP32S2_LOW_POWER_SENSOR_WEB_SOCKET_H
#define ESP32S2_LOW_POWER_SENSOR_WEB_SOCKET_H

#include "esp_http_server.h"

esp_err_t ws_handler(httpd_req_t *req);

#endif //ESP32S2_LOW_POWER_SENSOR_WEB_SOCKET_H
