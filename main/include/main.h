/***
** Created by Aleksey Volkov on 21.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_MAIN_H
#define HV_CC_LED_DRIVER_RTOS_MAIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define FIRMWARE "1.10"
#define HARDWARE "ESP32-PICO-D4"

/* Root cert for io.alab.cc, taken from io_alab_cc.pem
  The PEM file was extracted from the output of this command:
  openssl s_client -showcerts -connect io.alab.cc:443 </dev/null 2>/dev/null|openssl x509 -outform PEM >io_alab_cc.pem

  MQTT SSL:
  openssl s_client -showcerts -connect io.alab.cc:8883 </dev/null 2>/dev/null|openssl x509 -outform PEM >mqtt_io_alab_cc.pem
*/
/* HTTPS */
//extern const uint8_t http_server_cert_pem_start[] asm("_binary_io_alab_cc_pem_start");
//extern const uint8_t http_server_cert_pem_end[] asm("_binary_io_alab_cc_pem_end");

/* MQTTS */
//extern const uint8_t mqtt_server_cert_pem_start[] asm("_binary_mqtt_io_alab_cc_pem_start");
//extern const uint8_t mqtt_server_cert_pem_end[] asm("_binary_mqtt_io_alab_cc_pem_end");

extern EventGroupHandle_t wifi_event_group;

esp_err_t upgrade_firmware();

#endif //HV_CC_LED_DRIVER_RTOS_MAIN_H
