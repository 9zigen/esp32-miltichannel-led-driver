/***
** Created by Aleksey Volkov on 16.12.2019.
***/

#ifndef HV_CC_LED_DRIVER_RTOS_SETTINGS_H
#define HV_CC_LED_DRIVER_RTOS_SETTINGS_H

#include "stdbool.h"
#include <esp_system.h>
#include <driver/gpio.h>
#include "board.h"

/* Hardware */
#define HARDWARE_MODEL "PICO D4 5CH LED DRIVER AIO"
#define HARDWARE_MANUFACTURER "Alab"
#define HARDWARE_VERSION "1.0"

#define MAX_NETWORKS        2

#if defined(PICO_D4_5CH_LED_DRIVER_AIO)
#define MAX_LED_CHANNELS    5
#define BOARD_GPIO_MAP { GPIO_NUM_25, GPIO_NUM_34, GPIO_NUM_35 }
#define MAX_BOARD_GPIO 3
#elif defined(CUSTOM_7CH_LED_DRIVER_AIO)
#define MAX_LED_CHANNELS    7
#else
#define MAX_LED_CHANNELS    8
#endif

#define MAX_SCHEDULE        12
#define MAX_BRIGHTNESS      100

/* The range of the duty cycle values passed to functions depends on selected duty_
 * resolution and should be from 0 to (2 ** duty_resolution) - 1.
 * For example, if the selected duty resolution is 10, then the duty cycle values
 * can range from 0 to 1023. This provides the resolution of ~0.1%. */
#define DUTY_STEPS          2047 /* 5kHz = 13 bits  */

#define LED_SETTINGS_VERSION 0xA0
#define SCHEDULE_CONFIG_SETTINGS_VERSION 0xA2


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
  char hostname[32];                    // Device Name
  char ota_url[64];                     // OTA Server URL (full web path to firmware)
  char ntp_server[32];                  // Wifi SSID Name
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
  char token[32];                       // ThingsBoard Device Token
  char endpoint[64];                    // ThingsBoard MQTT Endpoint
  uint8_t qos;                          // MQTT QoS
  uint8_t retain;                       // MQTT Retain
  uint8_t enable;                       // ThingsBoard service
  uint8_t rpc;                          // Subscribe and process ThingsBoard RPC
} thingsboard_t;

typedef struct {
  uint8_t id;                           // channel ID
  char color[8];                        // RGB CSS Hex value FFFFFF -> white
  uint16_t power;                       // Real Channel Power in Watts x 10 (0-65535) 100 = 10.0 in Web UI
  uint8_t duty_max;                     // MAX Channel duty
  uint8_t sync_channel;                 // Enable/Disable channel sync
  uint8_t sync_channel_group;           // Sync channel group
  uint8_t state;                        // Enable/Disable channel
  uint8_t version;
} led_t;

typedef struct {
  uint8_t  time_hour;                   // Schedule fire hour
  uint8_t  time_minute;                 // Schedule fire minutes
  uint8_t  duty[MAX_LED_CHANNELS];      // Duty 0 - 255
  uint8_t  brightness;                  // All channels brightness in percentage (0-100%)
  bool     active;                      // Need send by WS to GUI
} schedule_t;

typedef struct {
  bool      installed;
  uint8_t   start_temp;
  uint8_t   target_temp;
  uint8_t   max_temp;
  double    pid_kp;   /* proportional gain 2.5 */
  double    pid_ki;   /* integral gain 1.0 */
  double    pid_kd;   /* derivative gain 1.0 */
} cooling_t;

typedef struct {
  char user[32];
  char password[32];
} auth_t;

typedef enum {
  SIMPLE, ADVANCED
} schedule_mode_t;

typedef struct {
  schedule_mode_t  mode;                // 0 -> simple sunrise/sunset; 1 -> multiple points
  bool     rgb;                         // RGB color picker
  uint8_t  duty_mode;                   // 0: 0-100%, 1: 0-255, 2: 0-1024
  uint8_t  sunrise_hour;
  uint8_t  sunrise_minute;
  uint8_t  sunset_hour;
  uint8_t  sunset_minute;
  uint8_t  simple_mode_duration;        // sunrise/sunset duration in minutes
  uint8_t  brightness;
  uint8_t  duty[MAX_LED_CHANNELS];      // Duty 0 - 255
  uint8_t  use_sync;
  uint8_t  sync_group;
  uint8_t  sync_master;
  uint8_t  gamma;
  uint8_t  version;
} schedule_config_t;

typedef enum {
  NA = 0,
  BRIGTNESS_UP, BRIGTNESS_DOWN, CHANGE_CHANNEL, APPLY
} board_gpio_func_t;

typedef struct {
  uint8_t pin;
  board_gpio_func_t function;
  board_gpio_func_t alt_function;
} board_gpio_config_t;

void init_settings(void);
void set_default_network(void);
void set_default_service(void);
void set_default_thingsboard(void);
void set_default_led(void);
void set_default_schedule(void);
void set_default_schedule_config(void);
void set_default_cooling(void);
void set_default_board_gpio_config(void);
void set_default_auth(void);

void set_network(void);
void set_service(void);
void set_thingsboard(void);
void set_led(void);
void set_schedule(void);
void set_schedule_config(void);
void set_cooling(void);
void set_board_gpio_config(void);
void set_auth(void);

void erase_settings(void);

network_t * get_networks(uint8_t network_id);
services_t * get_services(void);
thingsboard_t * get_thingsboard(void);
led_t * get_leds(uint8_t led_id);
schedule_t * get_schedule(uint8_t schedule_id);
schedule_config_t * get_schedule_config();
cooling_t * get_cooling();
board_gpio_config_t * get_board_gpio_config(uint8_t id);
auth_t * get_auth();

void ip_to_string(uint8_t ip[4], char* string);
void string_to_ip(const char *ip_string, uint8_t *octets);

#endif //HV_CC_LED_DRIVER_RTOS_SETTINGS_H
