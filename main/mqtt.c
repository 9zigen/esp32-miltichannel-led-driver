//
// Created by Aleksey Volkov on 13/5/23.
//

#include <cJSON.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "app_events.h"
#include "app_mqtt.h"
#include "mqtt.h"
#include "adc.h"
#include "app_settings.h"
#include "rtc.h"
#include "monitor.h"
#include "light.h"

ESP_EVENT_DECLARE_BASE(APP_EVENTS);
ESP_EVENT_DECLARE_BASE(APP_MQTT_EVENTS);

static const char *TAG = "MQTT";
const char * online = "online";
const char * offline = "offline";
const char * on = "ON";
const char * off = "OFF";

void mqtt_subscribe(esp_mqtt_client_handle_t mqtt_client);
void mqtt_on_message(esp_mqtt_event_handle_t incoming_event);
void mqtt_publish_device_status(esp_mqtt_client_handle_t mqtt_client);
void mqtt_publish_light_state(esp_mqtt_client_handle_t mqtt_client);


static void led_state_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    ESP_LOGD(TAG, "handling %s:%ld from", event_base, event_id);
    if (event_id == LIGHT_CHANGE_EVENT)
    {
        app_mqtt_publish_changes(event_id);
    }
}

void start_mqtt()
{
    /* MQTT Task */
    xTaskCreate(&app_mqtt_task, "app_mqtt_task", 6144, NULL, 5, NULL);

    /* Register event listener */
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(app_event_loop,
                                                             APP_EVENTS,
                                                             LIGHT_CHANGE_EVENT,
                                                             led_state_event_handler,
                                                             NULL,
                                                             NULL));
}

/* Subscribe to command topics */
void mqtt_subscribe(esp_mqtt_client_handle_t mqtt_client)
{
    int msg_id;
    char topic[128];

    services_t * settings = get_services();

    /* Subscribe to Single Channel Brightness topic
     * [hostname]/light/[channel_number]/set
     * */
    for (int i = 0; i < MAX_LED_CHANNELS + 1; ++i)
    {
        /* make topic string */
        snprintf(topic, 128, "%s/light/%d/set", settings->hostname, i);

        /* subscribe to topic QoS */
        msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, settings->mqtt_qos);
        ESP_LOGD(TAG, "subscribe to topic: %s", topic);
        ESP_LOGD(TAG, "sent subscribe successful, msg_id=%d", msg_id);
    }
}

/* On Message */
/* Topic examples:
 * SET Global Brightness:           LED_11324571/light/99/set      | payload JSON: { "brightness": 255 }
 * SET Single Channel Brightness:   LED_11324571/light/0/set       | payload JSON: { "brightness": 255 }
 * */
void mqtt_on_message(esp_mqtt_event_handle_t incoming_event)
{
    ESP_LOGD(TAG, "publish received:");
    ESP_LOGD(TAG,"TOPIC=%.*s", incoming_event->topic_len, incoming_event->topic);
    ESP_LOGD(TAG,"DATA=%.*s", incoming_event->data_len, incoming_event->data);

    char topic_template_buf[128];
    char topic_buf[128];
    char data_buf[128];
    int cmd = -1;
    uint8_t new_brightness = 0;
    uint32_t channel = 0xFFFFFFFF;

    snprintf(topic_buf, 128, "%.*s", incoming_event->topic_len, incoming_event->topic);
    snprintf(data_buf, 128, "%.*s", incoming_event->data_len, incoming_event->data);

    services_t * settings = get_services();

    /* check SET command */
    /* "sscanf" template example: LED_11324571/light/%u/set */
    snprintf(topic_template_buf, 128, "%s/light/%%u/set", settings->hostname);
    sscanf(topic_buf, topic_template_buf, &channel);

    /* incorrect channel received */
    if (channel == 0xFFFFFFFF)
        return;

    /* Parse JSON payload */
    cJSON * root = cJSON_Parse(data_buf);
    cJSON * state = cJSON_GetObjectItem(root, "state");
    cJSON * brightness = cJSON_GetObjectItem(root, "brightness");

    if (cJSON_IsString(state) && (state->valuestring != NULL)) {
        if (strncmp(state->valuestring, "ON", 2) == 0)
        {
            cmd = 1;
        }
        else if (strncmp(state->valuestring, "OFF", 3) == 0)
        {
            cmd = 0;
        }
    }

    if (cJSON_IsNumber(brightness))
    {
        new_brightness = brightness->valueint;
    }

    /* Set full brightness if received only State ON */
    if (cmd == 1 && new_brightness == 0)
    {
        new_brightness = 255;
    }

    /* Clean Up */
    cJSON_Delete(root);

    /* Overall Brightness received */
    if (channel == 0)
    {
        ESP_LOGD(TAG,"set overall brightness: %u | %d\n", new_brightness, cmd);
        set_brightness(new_brightness, 0);

        /* update topics */
//        mqtt_publish_light_state(incoming_event->client);
        return;
    }
    else
    {
        ESP_LOGD(TAG,"set channel brightness: %u | %u channel: %lu\n", new_brightness, cmd, channel);
        set_channel_duty(channel - 1, new_brightness, 0);
    }
}

/* Publish led channels current state
 * [hostname]/light/[channel_number]/state
 * */
void mqtt_publish_light_state(esp_mqtt_client_handle_t mqtt_client)
{
    int msg_id;
    char topic[128];
    char * message_buf;

    services_t * settings = get_services();

    /* light channels */
    for (uint8_t i = 0; i < MAX_LED_CHANNELS + 1; ++i)
    {
        /* topic */
        snprintf(topic, 128, "%s/light/%d/state", settings->hostname, i);

        /* JSON payload */
        cJSON * root = cJSON_CreateObject();

        if (i == 0)
        {
            /* first channel is global brightness level */
            cJSON_AddItemToObject(root, "brightness", cJSON_CreateNumber(get_brightness()));
            cJSON_AddItemToObject(root, "state", cJSON_CreateString( get_light_state()? "ON" : "OFF"));
        }
        else
        {
            cJSON_AddItemToObject(root, "brightness", cJSON_CreateNumber(get_channel_duty(i - 1)));
            cJSON_AddItemToObject(root, "state", cJSON_CreateString( get_channel_state(i - 1)? "ON" : "OFF"));
        }


        message_buf = cJSON_Print(root);
        if (message_buf == NULL)
        {
            ESP_LOGE(TAG, "failed to print light state json.");
        }

        cJSON_Delete(root);

        /* publish led status to topic */
        msg_id = esp_mqtt_client_publish(mqtt_client, topic, message_buf, strlen(message_buf),
                                         settings->mqtt_qos, 0);
        free(message_buf);
        ESP_LOGD(TAG, "publish to: %s, msg: %s", topic, message_buf);
        ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

/* Publish device status
 * [hostname]
 *
 * Light channels status
 * [hostname]/light/%u/available
 * */
void mqtt_publish_device_status(esp_mqtt_client_handle_t mqtt_client)
{
    char * p;
    int msg_id;
    char topic[128];
    char * message_buf;
    char time_string[32];

    /* Device DB */
    services_t * services = get_services();


    /* Light Channels availability */
    for (uint8_t light_idx = 0; light_idx < MAX_LED_CHANNELS + 1; ++light_idx)
    {
        /* topic */
        snprintf(topic, 128, "%s/light/%u/available", services->hostname, light_idx);

        if (light_idx == 0)
        {
            /* Overall brightness channel */
            p = (char *)online;
        }
        else
        {
            /* Load light channel info from DB */
            led_t * led_channel = get_leds(light_idx - 1);

            /* message */
            if (led_channel->state)
                p = (char *)online;
            else
                p = (char *)offline;
        }

        msg_id = esp_mqtt_client_publish(mqtt_client, topic, p, (int)strlen(p), 1, 0);
        ESP_LOGD(TAG, "publish to: %s", topic);
        ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);
    }

    /* Device Status */
    system_status_t * system_status = get_system_status();

    /* voltage */
    uint32_t voltage_mv = (uint32_t)read_vcc_voltage();
    double voltage = voltage_mv / 1000.0;

    /* Power calculation */
    double power = 0;
    uint8_t brightness = get_brightness();
    for (int i = 0; i < MAX_LED_CHANNELS; ++i)
    {
        led_t * channel = get_leds(i);
        if (channel->state)
        {
            double percent = (double) channel->power * (get_channel_duty(i) / 255.0);
            ESP_LOGD(TAG, "channel power: %f", percent);
            percent = percent * (brightness / 255.0);
            ESP_LOGD(TAG, "channel power corrected: %f", percent);

            power += percent;
        }
    }

    power = (uint32_t)(power * 100.0);
    power = power / 100.0;

    /* topic */
    snprintf(topic, 128, "%s", services->hostname);

    /* JSON payload */
    cJSON *root = cJSON_CreateObject();

    det_time_string_since_boot((char*) &time_string);
    cJSON_AddItemToObject(root, "state", cJSON_CreateString(online));
    cJSON_AddItemToObject(root, "up_time", cJSON_CreateString(time_string));
    get_time_string((char*) &time_string);
    cJSON_AddItemToObject(root, "local_time", cJSON_CreateString(time_string));
    cJSON_AddItemToObject(root, "free_heap", cJSON_CreateNumber(system_status->free_heap));
    cJSON_AddItemToObject(root, "voltage", cJSON_CreateNumber(voltage));
    cJSON_AddItemToObject(root, "power", cJSON_CreateNumber(power));
    cJSON_AddItemToObject(root, "temperature", cJSON_CreateNumber(read_ntc_temperature()));

////  cJSON_AddItemToObject(root, "board_temperature", cJSON_CreateNumber(get_stm_mcu_temperature()));
    cJSON_AddItemToObject(root, "local_ip_address", cJSON_CreateString(system_status->net_address));
    cJSON_AddItemToObject(root, "mac_address", cJSON_CreateString(system_status->mac));

    message_buf = cJSON_Print(root);
    if (message_buf == NULL)
    {
        ESP_LOGE(TAG, "failed to print device status json.");
    }

    cJSON_Delete(root);

    /* publish led status to topic */
    msg_id = esp_mqtt_client_publish(mqtt_client, topic, message_buf, strlen(message_buf),
                                     services->mqtt_qos, 0);
    free(message_buf);
    ESP_LOGD(TAG, "publish to: %s", topic);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);
}