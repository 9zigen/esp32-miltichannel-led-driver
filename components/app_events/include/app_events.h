//
// Created by Aleksey Volkov on 2/10/22.
//

#ifndef ESP32_CC_LED_DRIVER_RTOS_ALAB_EVENT_BUS_H
#define ESP32_CC_LED_DRIVER_RTOS_ALAB_EVENT_BUS_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(APP_EVENTS);


typedef enum {
    UNDEFINED_EVENT = 0,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    TEMPERATURE_CHANGE_EVENT,
    LIGHT_CHANGE_EVENT
} app_event_t;

typedef void (*app_event_handler_t)(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

extern esp_event_loop_handle_t app_event_loop;

esp_err_t init_events();
void notify_app(app_event_t event, const void* event_data, size_t event_data_size);

#endif //ESP32_CC_LED_DRIVER_RTOS_ALAB_EVENT_BUS_H
