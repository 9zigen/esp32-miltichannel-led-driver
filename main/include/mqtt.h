/***
** Created by Aleksey Volkov on 01.01.2020.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_MQTT_H
#define HV_CC_LED_DRIVER_RTOS_MQTT_H

typedef enum {
  MQTT_DISABLED, MQTT_ENABLED_NOT_CONNECTED, MQTT_ENABLED_CONNECTED
} mqtt_service_status_t ;

void task_mqtt(void *pvParameters);

void init_mqtt(void);
void mqtt_publish_brightness();
void mqtt_publish_channel_duty();
void mqtt_publish_channel_state();
void mqtt_publish_device_status();

/* cloud */
void init_thingsboard();
void mqtt_cloud_publish_device_status();

mqtt_service_status_t get_mqtt_status();
#endif //HV_CC_LED_DRIVER_RTOS_MQTT_H
