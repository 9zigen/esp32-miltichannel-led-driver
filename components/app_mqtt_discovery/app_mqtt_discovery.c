//
// Created by Aleksey Volkov on 10/5/23.
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <cJSON.h>
#include <esp_err.h>
#include <esp_log.h>
#include <mqtt_client.h>

#include "app_settings.h"
#include "app_mqtt_discovery_sensors.h"
#include "app_mqtt_discovery.h"

#define ATTR_NAME "name"
#define ATTR_MODEL "model"
#define ATTR_IDENTIFIERS "identifiers"
#define ATTR_MANUFACTURER "manufacturer"
#define ATTR_SW_VER "sw_version"
#define ATTR_HW_VER "hw_version"
#define ATTR_VIA_DEVICE "via_device"
#define ATTR_DEVICE "device"
#define ATTR_UID "unique_id"
#define ATTR_STATE_TOPIC "state_topic"
#define ATTR_COMMAND_TOPIC "command_topic"
#define ATTR_AVAILABILITY "availability"
#define ATTR_PAYLOAD_NOT_AVAILABLE "payload_not_available"
#define ATTR_PAYLOAD_AVAILABLE "payload_available"
#define ATTR_TOPIC "topic"
#define ATTR_SCHEMA "schema"
#define ATTR_CONFIGURATION_URL "configuration_url"
#define ATTR_BRIGHTNESS "brightness"
#define ATTR_COLOR_MODE "color_mode"
#define ATTR_SUPPORTED_COLOR_MODES "supported_color_modes"
#define ATTR_VALUE_TEMPLATE "value_template"
#define ATTR_STATE_CLASS "state_class"
#define ATTR_UNIT_OF_MEASUREMENT "unit_of_measurement"
#define ATTR_ENABLED_BY_DEFAULT "enabled_by_default"

#define VALUE_BRIGHTNESS "brightness"
#define VALUE_JSON "json"
#define VALUE_NOT_AVAILABLE "offline"
#define VALUE_AVAILABLE "online"
#define VALUE_DIAGNOSTIC "diagnostic"
#define VALUE_TRUE "true"

static const char *TAG = "APP MQTT DISCOVERY";
static const discovery_settings_t *cfg;

static device_discovery_data_t get_sensor(device_discovery_sensor_t type)
{
    device_discovery_data_t response = {
            .topic = NULL,
            .payload = NULL
    };

    char topic_buff[128];
    char device_path[64];
    const char * device_class = app_mqtt_discovery_sensors_get_device_class(type);
    const char * state_class = app_mqtt_discovery_sensors_get_state_class(type);
    const char * units = app_mqtt_discovery_sensors_get_units(type);

    snprintf(device_path, 64, "%s", cfg->device_id);
    snprintf(topic_buff, 128, "%s/sensor/%s/%s/config", cfg->hass_prefix, device_path, device_class);

    response.topic = malloc(128);
    memset(response.topic, 0x00, 128);
    strlcpy(response.topic, topic_buff, 128);

    char entity_name[64];
    char entity_state_topic[128];
    char entity_availability_topic[128];
    char entity_uid[64];
    char entity_value_template[64];
    char device_configuration_url[128];

    snprintf(entity_name, 64, "%s", device_class);
    snprintf(entity_state_topic, 128, "%s", cfg->device_id);
    snprintf(entity_availability_topic, 128, "%s", cfg->device_id);
    snprintf(entity_uid, 64, "%s_%s", cfg->device_id, device_class);
    snprintf(entity_value_template, 64, "{{ value_json.%s }}", device_class);
    snprintf(device_configuration_url, 64, "http://%s.local", cfg->device_id);

    /* Entity configuration */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, ATTR_NAME, cJSON_CreateString(entity_name));
    cJSON_AddItemToObject(root, ATTR_UID, cJSON_CreateString(entity_uid));
    cJSON_AddItemToObject(root, ATTR_STATE_TOPIC, cJSON_CreateString(entity_state_topic));
    cJSON_AddItemToObject(root, ATTR_VALUE_TEMPLATE, cJSON_CreateString(entity_value_template));
    cJSON_AddItemToObject(root, ATTR_STATE_CLASS, cJSON_CreateString(state_class));
    cJSON_AddItemToObject(root, ATTR_UNIT_OF_MEASUREMENT, cJSON_CreateString(units));
    cJSON_AddItemToObject(root, ATTR_ENABLED_BY_DEFAULT, cJSON_CreateString(VALUE_TRUE));

    cJSON *availability = cJSON_CreateArray();
    cJSON *availability_item = cJSON_CreateObject();
    cJSON_AddItemToObject(availability_item, ATTR_TOPIC, cJSON_CreateString(entity_availability_topic));
    cJSON_AddItemToObject(availability_item, ATTR_PAYLOAD_NOT_AVAILABLE, cJSON_CreateString(VALUE_NOT_AVAILABLE));
    cJSON_AddItemToObject(availability_item, ATTR_PAYLOAD_AVAILABLE, cJSON_CreateString(VALUE_AVAILABLE));
    cJSON_AddItemToObject(availability_item, ATTR_VALUE_TEMPLATE, cJSON_CreateString("{{ value_json.state }}"));

    cJSON_AddItemToArray(availability, availability_item);
    cJSON_AddItemToObject(root, ATTR_AVAILABILITY, availability);

    /* Parent configuration */
    cJSON *device = cJSON_CreateObject();
    cJSON_AddItemToObject(device, ATTR_NAME, cJSON_CreateString(cfg->device_name));
    cJSON_AddItemToObject(device, ATTR_MODEL, cJSON_CreateString(cfg->device_model));
    cJSON_AddItemToObject(device, ATTR_MANUFACTURER, cJSON_CreateString(cfg->device_manufacturer));
    cJSON_AddItemToObject(device, ATTR_SW_VER, cJSON_CreateString(cfg->device_sw_version));
    cJSON_AddItemToObject(device, ATTR_HW_VER, cJSON_CreateString(cfg->device_hw_version));
    cJSON *identifiers = cJSON_CreateArray();
    cJSON_AddItemToArray(identifiers, cJSON_CreateString(cfg->device_model));
    cJSON_AddItemToArray(identifiers, cJSON_CreateString(cfg->device_id));
    cJSON_AddItemToObject(device, ATTR_IDENTIFIERS, identifiers);
    cJSON_AddItemToObject(device, ATTR_VIA_DEVICE, cJSON_CreateString(cfg->device_name));
    cJSON_AddItemToObject(device, ATTR_CONFIGURATION_URL, cJSON_CreateString(device_configuration_url));
    cJSON_AddItemToObject(root, ATTR_DEVICE, device);

    response.payload = cJSON_Print(root);

    cJSON_Delete(root);

    return response;
}

/**
 *
 * @param light_id sequence number in the group
 * @param topic pointer to char array
 * @param payload pointer to char array
 */
static device_discovery_data_t get_light(uint8_t light_idx)
{
    device_discovery_data_t response = {
            .topic = NULL,
            .payload = NULL
    };

    char topic_buff[128];
    char device_path[64];

    snprintf(device_path, 64, "%s-%u", cfg->device_id, light_idx);
    snprintf(topic_buff, 128, "%s/light/%s/light/config", cfg->hass_prefix, device_path);

    response.topic = malloc(128);
    memset(response.topic, 0x00, 128);
    strlcpy(response.topic, topic_buff, 128);

    char entity_name[64];
    char entity_state_topic[128];
    char entity_command_topic[128];
    char entity_availability_topic[128];
    char entity_uid[64];
    char device_configuration_url[128];

    snprintf(entity_name, 64, "Led channel %u", light_idx);
    snprintf(entity_state_topic, 128, "%s/light/%u/state", cfg->device_id, light_idx);
    snprintf(entity_command_topic, 128, "%s/light/%u/set", cfg->device_id, light_idx);
    snprintf(entity_availability_topic, 128, "%s/light/%u/available", cfg->device_id, light_idx);
    snprintf(entity_uid, 64, "%s_light_%u", cfg->device_id, light_idx);
    snprintf(device_configuration_url, 64, "http://%s.local", cfg->device_id);

    /* Entity configuration */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, ATTR_NAME, cJSON_CreateString(entity_name));
    cJSON_AddItemToObject(root, ATTR_UID, cJSON_CreateString(entity_uid));
    cJSON_AddItemToObject(root, ATTR_STATE_TOPIC, cJSON_CreateString(entity_state_topic));
    cJSON_AddItemToObject(root, ATTR_COMMAND_TOPIC, cJSON_CreateString(entity_command_topic));
    cJSON_AddItemToObject(root, ATTR_SCHEMA, cJSON_CreateString(VALUE_JSON));
    cJSON_AddItemToObject(root, ATTR_BRIGHTNESS, cJSON_CreateBool(true));
    cJSON_AddItemToObject(root, ATTR_COLOR_MODE, cJSON_CreateBool(true));
    cJSON_AddItemToObject(root, ATTR_SUPPORTED_COLOR_MODES, cJSON_CreateString(VALUE_BRIGHTNESS));

    cJSON *availability = cJSON_CreateArray();
    cJSON *availability_item = cJSON_CreateObject();
    cJSON_AddItemToObject(availability_item, ATTR_TOPIC, cJSON_CreateString(entity_availability_topic));
    cJSON_AddItemToObject(availability_item, ATTR_PAYLOAD_NOT_AVAILABLE, cJSON_CreateString(VALUE_NOT_AVAILABLE));
    cJSON_AddItemToObject(availability_item, ATTR_PAYLOAD_AVAILABLE, cJSON_CreateString(VALUE_AVAILABLE));
    cJSON_AddItemToArray(availability, availability_item);
    cJSON_AddItemToObject(root, ATTR_AVAILABILITY, availability);

    /* Parent configuration */
    cJSON *device = cJSON_CreateObject();
    cJSON_AddItemToObject(device, ATTR_NAME, cJSON_CreateString(cfg->device_name));
    cJSON_AddItemToObject(device, ATTR_MODEL, cJSON_CreateString(cfg->device_model));
    cJSON_AddItemToObject(device, ATTR_MANUFACTURER, cJSON_CreateString(cfg->device_manufacturer));
    cJSON_AddItemToObject(device, ATTR_SW_VER, cJSON_CreateString(cfg->device_sw_version));
    cJSON_AddItemToObject(device, ATTR_HW_VER, cJSON_CreateString(cfg->device_hw_version));
    cJSON *identifiers = cJSON_CreateArray();
    cJSON_AddItemToArray(identifiers, cJSON_CreateString(cfg->device_model));
    cJSON_AddItemToArray(identifiers, cJSON_CreateString(cfg->device_id));
    cJSON_AddItemToObject(device, ATTR_IDENTIFIERS, identifiers);
    cJSON_AddItemToObject(device, ATTR_VIA_DEVICE, cJSON_CreateString(cfg->device_name));
    cJSON_AddItemToObject(device, ATTR_CONFIGURATION_URL, cJSON_CreateString(device_configuration_url));
    cJSON_AddItemToObject(root, ATTR_DEVICE, device);

    response.payload = cJSON_Print(root);

    cJSON_Delete(root);

    return response;
}

/**
 *
 * @param mqtt_client_instance
 * @param qos
 * @param retain
 */
void hass_mqtt_discovery_configure_device(esp_mqtt_client_handle_t mqtt_client_instance, int qos, int retain)
{
    int msg_id;
    device_discovery_data_t data;

    /* Light channels */
    for (int i = 0; i < MAX_LED_CHANNELS + 1; ++i)
    {
        /* Get device configuration JSON */
        data = get_light(i);

        /* Publish configuration */
        if (data.topic != NULL && data.payload != NULL)
        {
            msg_id = esp_mqtt_client_publish(mqtt_client_instance,
                                             data.topic,
                                             data.payload,
                                             strlen(data.payload), qos, retain);
            ESP_LOGD(TAG, "publish to: %s, msg: %s", data.topic, data.payload);
            ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);
        } else {
            ESP_LOGE(TAG, "get_light data NULL");
        }

        free(data.topic);
        free(data.payload);
    }

    /* Sensors */
    const device_discovery_sensor_t sensors_list[3] =
            {DISCOVERY_POWER_SENSOR,
             DISCOVERY_TEMPERATURE_SENSOR,
             DISCOVERY_VOLTAGE_SENSOR};

    for (int i = 0; i < 3; ++i) {
        data = get_sensor(sensors_list[i]);
        msg_id = esp_mqtt_client_publish(mqtt_client_instance,
                                         data.topic,
                                         data.payload,
                                         strlen(data.payload), qos, retain);
        ESP_LOGD(TAG, "publish to: %s, msg: %s", data.topic, data.payload);
        ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);

        free(data.topic);
        free(data.payload);
    }
}

/**
 *
 * @param params
 * @return
 */
esp_err_t hass_mqtt_discovery_init(const discovery_settings_t * params)
{
    if (!strlen(params->device_id) || !strlen(params->hass_prefix))
    {
        return ESP_FAIL;
    }

    cfg = params;

    return ESP_OK;
}

void hass_mqtt_discovery_deinit()
{
    free(cfg->device_id);
    free(cfg->device_model);
    free(cfg->device_name);
    free(cfg->device_manufacturer);
    free(cfg->device_sw_version);
    free(cfg->device_hw_version);
    free(cfg->hass_prefix);
}