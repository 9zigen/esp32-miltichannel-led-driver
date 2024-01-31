//
// Created by Aleksey Volkov on 10/5/23.
//

#ifndef HASS_MQTT_DISCOVERY_H
#define HASS_MQTT_DISCOVERY_H

#include <esp_err.h>

typedef struct {
    char * device_id;             /* [hostname] Identifier of a device that routes messages between this device and Home Assistant */
    char * device_model;          /* The model of the device. */
    char * device_name;           /* Name of this device */
    char * device_manufacturer;   /* The manufacturer of the device. */
    char * device_sw_version;     /* The firmware version of the device. */
    char * device_hw_version;     /* The hardware version of the device. */
    char * hass_prefix;           /* Home Assistant MQTT prefix */
} discovery_settings_t;

typedef struct {
    char * topic;
    char * payload;
} device_discovery_data_t;

esp_err_t hass_mqtt_discovery_init(const discovery_settings_t * params);
void hass_mqtt_discovery_deinit();
void hass_mqtt_discovery_configure_device(esp_mqtt_client_handle_t mqtt_client_instance, int qos, int retain);

#endif //HASS_MQTT_DISCOVERY_H
