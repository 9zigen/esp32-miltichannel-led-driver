/***
** Created by Aleksey Volkov on 01.01.2020.
***/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <cJSON.h>
#include <mqtt_client.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_app_desc.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "app_settings.h"
#include "app_mqtt_discovery.h"
#include "app_mqtt.h"
#include "app_events.h"

static const char *TAG = "APP MQTT";

esp_mqtt_client_handle_t client;
uint8_t mqtt_enabled = 0;
uint8_t mqtt_connected = 0;

QueueHandle_t publish_queue;
TimerHandle_t timer_publish_status;

ESP_EVENT_DECLARE_BASE(APP_MQTT_EVENTS);

/* External function defined in mqtt.c */
extern void mqtt_subscribe(esp_mqtt_client_handle_t mqtt_client);
extern void mqtt_on_message(esp_mqtt_event_handle_t incoming_event);
extern void mqtt_publish_device_status(esp_mqtt_client_handle_t mqtt_client);
extern void mqtt_publish_light_state(esp_mqtt_client_handle_t mqtt_client);

/* MQTT Event Handler */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
//  esp_mqtt_client_handle_t client = event->client;
//  int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
        case MQTT_EVENT_CONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = 1;
            /* start device status timer */
            xTimerStart(timer_publish_status, 10 / portTICK_PERIOD_MS);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_connected = 0;
            xTimerStop(timer_publish_status, 10 / portTICK_PERIOD_MS);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
            mqtt_on_message(event);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
            break;
        case MQTT_EVENT_DELETED:
            ESP_LOGD(TAG, "MQTT_EVENT_DELETED");
            break;
        default:
            ESP_LOGD(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void vTimerCallback( TimerHandle_t xTimer )
{
    mqtt_publish_device_status(client);
}

/**
 *
 * @param params mqtt_settings_t
 * @return use free()
 */
static char * get_uri_string(mqtt_settings_t * params)
{
    char prefix[8];

    switch (params->transport) {
        case MQTT_TRANSPORT_OVER_TCP:
            strlcpy(prefix, "mqtt", 8);
            break;
        case MQTT_TRANSPORT_OVER_SSL:
            strlcpy(prefix, "mqtts", 8);
            break;
        case MQTT_TRANSPORT_OVER_WS:
            strlcpy(prefix, "ws", 8);
            break;
        case MQTT_TRANSPORT_OVER_WSS:
            strlcpy(prefix, "wss", 8);
            break;
        default:
            strlcpy(prefix, "mqtt", 8);
            break;
    }

    char * ip_addr;
    ip_addr = malloc(IP4ADDR_STRLEN_MAX);
    ip4addr_ntoa_r(params->server_ip_v4, ip_addr, IP4ADDR_STRLEN_MAX);

    char * uri = malloc(64);
    snprintf(uri, 64, "%s://%s:%u", prefix, ip_addr, params->server_port);

    free(ip_addr);
    return uri;
}

/* Publish changes */
esp_err_t app_mqtt_publish_changes(int32_t event_id)
{
    if (publish_queue == 0)
        return ESP_ERR_NO_MEM;

    xQueueSendToBack(publish_queue, &event_id, 100 / portTICK_PERIOD_MS);
    return ESP_OK;
}

/* Setup MQTT Client
 * .uri = "mqtts://api.emitter.io:443" for mqtt over ssl
 * .uri = "mqtt://api.emitter.io:8080" for mqtt over tcp
 * .uri = "ws://api.emitter.io:8080"   for mqtt over websocket
 * .uri = "wss://api.emitter.io:443"   for mqtt over websocket secure
 * */
esp_err_t init_mqtt(mqtt_settings_t * params)
{
    char * uri = get_uri_string(params);

    ESP_LOGD(TAG, "address: %s | user:%s | password:%s", uri, params->username, params->password);

    if (strlen(uri) > 20)
    {
        esp_mqtt_client_config_t mqtt_cfg = {
                .broker.address.uri = uri,
                .session.keepalive = 60
        };

        if (strlen(params->username)) {
            mqtt_cfg.credentials.username = strdup(params->username);
        }

        if (strlen(params->password)) {
            mqtt_cfg.credentials.authentication.password = strdup(params->password);
        }

        mqtt_enabled = 1;

        /* Create device status publish timer, 1 min period */
        timer_publish_status = xTimerCreate("TimerDeviceStatus", 30 * 1000 / portTICK_PERIOD_MS, pdTRUE, NULL, vTimerCallback);

        ESP_LOGI(TAG, "[APP] free memory before start MQTT client: %lu bytes", esp_get_free_heap_size());
        client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

        if (client != NULL)
        {
            esp_mqtt_client_start(client);
        }

        /* Remove before initialed strings by strdup */
        if (strlen(mqtt_cfg.credentials.username))
            free((char *)mqtt_cfg.credentials.username);

        if (strlen(mqtt_cfg.credentials.authentication.password))
            free((char *)mqtt_cfg.credentials.authentication.password);
    }

    free(uri);
    return ESP_OK;
}



void app_mqtt_task(void *pvParameters)
{
    /* Events queue */
    app_event_t event;
    publish_queue = xQueueCreate( 10, sizeof( app_event_t ) );

    /* Load service configuration */
    services_t * services = get_services();

    if (!services->enable_mqtt)
    {
        vTaskDelete(NULL);
    }

    mqtt_settings_t cfg = {
            .username = services->mqtt_user,
            .password = services->mqtt_password,
            .server_port = services->mqtt_port,
            .transport = MQTT_TRANSPORT_OVER_TCP
    };

    cfg.server_ip_v4 = malloc(sizeof(ip4_addr_t));
    memset(cfg.server_ip_v4, 0x00, sizeof(ip4_addr_t));
    IP4_ADDR(cfg.server_ip_v4,
             services->mqtt_ip_address[0],
             services->mqtt_ip_address[1],
             services->mqtt_ip_address[2],
             services->mqtt_ip_address[3]);

    /* Start MQTT client */
    init_mqtt(&cfg);

    while (!mqtt_connected) { vTaskDelay(1000 / portTICK_PERIOD_MS); }

    /* subscribe to channel brightness and global brightness topics */
    mqtt_subscribe(client);

    /* publish Home Assistant discovery data */
    const esp_app_desc_t * app_description = esp_app_get_description();
    discovery_settings_t discovery_cfg = {
            .device_id = strdup(services->hostname),
            .device_model = strdup(HARDWARE_MODEL),
            .device_name = strdup(services->hostname),
            .device_manufacturer = strdup(HARDWARE_MANUFACTURER),
            .device_sw_version = strdup(app_description->version),
            .device_hw_version = strdup(HARDWARE_VERSION),
            .hass_prefix = strdup("homeassistant")
    };

    if (hass_mqtt_discovery_init(&discovery_cfg) == ESP_OK)
    {
        hass_mqtt_discovery_configure_device(client, 1, 1);

        /* clean up used memory */
        hass_mqtt_discovery_deinit();
    } else {
        ESP_LOGE(TAG, "Home Assistant Discovery Init Failed");
    }

    /* publish current info */
    mqtt_publish_device_status(client);
    mqtt_publish_light_state(client);


    for (;;)
    {
        if( publish_queue != 0 )
        {
            if (xQueueReceive(publish_queue, &event, 200 / portTICK_PERIOD_MS))
            {
                if (event == LIGHT_CHANGE_EVENT)
                {
                    mqtt_publish_device_status(client);
                    mqtt_publish_light_state(client);
                }
            }
        }
    }
}

mqtt_service_status_t get_mqtt_status()
{
    if (mqtt_enabled && !mqtt_connected)
        return MQTT_ENABLED_NOT_CONNECTED;
    else if (mqtt_enabled && mqtt_connected)
        return MQTT_ENABLED_CONNECTED;
    else
        return MQTT_DISABLED;
}