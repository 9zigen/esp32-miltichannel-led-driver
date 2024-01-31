//
// Created by Aleksey Volkov on 15/5/23.
//

#ifndef APP_MQTT_DISCOVERY_SENSORS_H
#define APP_MQTT_DISCOVERY_SENSORS_H

typedef enum {
    DISCOVERY_POWER_SENSOR,
    DISCOVERY_ENERGY_SENSOR,
    DISCOVERY_TEMPERATURE_SENSOR,
    DISCOVERY_HUMIDITY_SENSOR,
    DISCOVERY_VOLTAGE_SENSOR,
    DISCOVERY_CURRENT_SENSOR
} device_discovery_sensor_t;

const char * app_mqtt_discovery_sensors_get_device_class(device_discovery_sensor_t type);
const char * app_mqtt_discovery_sensors_get_state_class(device_discovery_sensor_t type);
const char * app_mqtt_discovery_sensors_get_units(device_discovery_sensor_t type);

#endif //APP_MQTT_DISCOVERY_SENSORS_H
