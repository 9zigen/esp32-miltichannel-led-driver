/***
** Created by Aleksey Volkov on 01.01.2020.
***/

#ifndef ALAB_MQTT_H
#define ALAB_MQTT_H

#include <lwip/ip4_addr.h>
#include <mqtt_client.h>

typedef enum {
  MQTT_DISABLED, MQTT_ENABLED_NOT_CONNECTED, MQTT_ENABLED_CONNECTED
} mqtt_service_status_t ;

typedef struct {
    esp_mqtt_transport_t transport;
    ip4_addr_t * server_ip_v4;
    uint16_t server_port;
    char * username;
    char * password;
} mqtt_settings_t;

void app_mqtt_task(void *pvParameters);
esp_err_t app_mqtt_publish_changes(int32_t event_id);

esp_err_t init_mqtt(mqtt_settings_t * params);
mqtt_service_status_t get_mqtt_status();

#endif //ALAB_MQTT_H
