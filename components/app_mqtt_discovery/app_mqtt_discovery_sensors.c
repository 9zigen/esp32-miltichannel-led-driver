#include "app_mqtt_discovery_sensors.h"

static const char *empty = "empty";

/* device class */
static const char *power = "power";
static const char *energy = "energy";
static const char *temperature = "temperature";
static const char *humidity = "humidity";
static const char *voltage = "voltage";
static const char *current  = "current";

/* state class */
static const char *measurement  = "measurement";
static const char *total_increasing  = "total_increasing";

/* units */
static const char *celsius  = "°C";
static const char *kWh  = "kWh";
static const char *watt  = "W";
static const char *volt  = "V";
static const char *ampere  = "A";
static const char *percent  = "%";

const char * app_mqtt_discovery_sensors_get_device_class(device_discovery_sensor_t type)
{
    switch (type) {
        case DISCOVERY_POWER_SENSOR:
            return power;
        case DISCOVERY_ENERGY_SENSOR:
            return energy;
        case DISCOVERY_TEMPERATURE_SENSOR:
            return temperature;
        case DISCOVERY_HUMIDITY_SENSOR:
            return humidity;
        case DISCOVERY_VOLTAGE_SENSOR:
            return voltage;
        case DISCOVERY_CURRENT_SENSOR:
            return current;
    }

    return empty;
}

const char * app_mqtt_discovery_sensors_get_state_class(device_discovery_sensor_t type)
{
    switch (type) {
        case DISCOVERY_POWER_SENSOR:
            return measurement;
        case DISCOVERY_ENERGY_SENSOR:
            return total_increasing;
        case DISCOVERY_TEMPERATURE_SENSOR:
            return measurement;
        case DISCOVERY_HUMIDITY_SENSOR:
            return measurement;
        case DISCOVERY_VOLTAGE_SENSOR:
            return measurement;
        case DISCOVERY_CURRENT_SENSOR:
            return measurement;
    }

    return empty;
}

const char * app_mqtt_discovery_sensors_get_units(device_discovery_sensor_t type)
{
    switch (type) {
        case DISCOVERY_POWER_SENSOR:
            return watt;
        case DISCOVERY_ENERGY_SENSOR:
            return kWh;
        case DISCOVERY_TEMPERATURE_SENSOR:
            return celsius;
        case DISCOVERY_HUMIDITY_SENSOR:
            return percent;
        case DISCOVERY_VOLTAGE_SENSOR:
            return volt;
        case DISCOVERY_CURRENT_SENSOR:
            return ampere;
    }

    return empty;
}