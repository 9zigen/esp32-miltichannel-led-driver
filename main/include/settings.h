/***
** Created by Aleksey Volkov on 16.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_SETTINGS_H
#define HV_CC_LED_DRIVER_RTOS_SETTINGS_H

#include "stdbool.h"

#define MAX_NETWORKS        2
#define MAX_LED_CHANNELS    8
#define MAX_SCHEDULE        5
#define MAX_BRIGHTNESS      100
#define MAX_DUTY            MAX_BRIGHTNESS

typedef struct {
  char ssid[32];                        // Wifi SSID Name
  char password[64];                    // Wifi Password
  uint8_t ip_address[4];                // IP Address
  uint8_t mask[4];                      // Mask
  uint8_t gateway[4];                   // Gateway
  uint8_t dns[4];                       // DNS
  bool dhcp;                            // Enable DHCP Client
  bool active;                          // Need send by WS to GUI
} network_t;

typedef struct {
  char hostname[20];                    // Device Name
  char ntp_server[20];                  // Wifi SSID Name
  int16_t utc_offset;                   // UTC offset in minutes
  bool ntp_dst;                         // Daylight save
  uint8_t mqtt_ip_address[4];           // IP v4 Address Array
  uint16_t mqtt_port;                   // MQTT Server port 1883 default
  char mqtt_user[16];                   // 16 Char MAX
  char mqtt_password[16];               // 16 Char MAX
  uint8_t mqtt_qos;                     // MQTT QoS
  uint8_t mqtt_retain;                  // MQTT Retain
  bool enable_ntp;                      // Enable NTP Service
  bool enable_mqtt;                     // Enable MQTT Service
} services_t;

typedef struct {
  uint8_t id;                           // channel ID
  char color[8];                        // RGB CSS Hex value FFFFFF -> white
  uint16_t power;                       // Real Channel Power in Watts x 10 (0-65535) 100 = 10.0 in Web UI
  uint8_t state;                        // Enable/Disable channel
} led_t;

typedef struct {
  uint8_t  magic_number;
  uint8_t  time_hour;                   // Schedule fire hour
  uint8_t  time_minute;                 // Schedule fire minutes
  uint8_t  duty[MAX_LED_CHANNELS];      // Duty in percentage (0-100%)
  uint8_t  brightness;                  // All channels brightness in percentage (0-100%)
  bool     active;                      // Need send by WS to GUI
} schedule_t;

void init_settings(void);
void set_default_network(void);
void set_default_service(void);
void set_default_led(void);
void set_default_schedule(void);

void set_network(void);
void set_service(void);
void set_led(void);
void set_schedule(void);
void set_all_settings(void);

void erase_settings(void);

network_t * get_network_config(uint8_t network_id);
services_t * get_service_config(void);
led_t * get_led_config(uint8_t led_id);
schedule_t * get_schedule_config(uint8_t schedule_id);

void ip_to_string(uint8_t ip[4], char* string);
void string_to_ip(const char *ip_string, uint8_t *octets);

#endif //HV_CC_LED_DRIVER_RTOS_SETTINGS_H
