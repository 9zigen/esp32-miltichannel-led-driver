/***
** Created by Aleksey Volkov on 16.12.2019.
***/
#include <stdio.h>
#include <esp_log.h>
#include <esp_system.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "board.h"
#include "app_settings.h"

static const char *TAG = "APP SETTINGS";
const gpio_num_t board_gpio_map[MAX_BOARD_GPIO] = BOARD_GPIO_MAP;
const char * empty_str = "empty";

static network_t network[MAX_NETWORKS];
static services_t service;
static thingsboard_t thingsboard;
static led_t led[MAX_LED_CHANNELS];
static schedule_t schedule[MAX_SCHEDULE];
static schedule_config_t schedule_config;
static cooling_t cooling;
static board_gpio_config_t board_gpio_config[MAX_BOARD_GPIO];
static auth_t auth;

/* Initialize Settings */
void init_settings()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Initialize NVS */
  err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
    ESP_LOGE(TAG, "NVS partition was truncated and needs to be erased");

    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK( err );

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {

    /* Read Networks */
    ESP_LOGI(TAG, "Reading Networks ...");
    size_t required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "network", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size != (sizeof(network_t) * MAX_NETWORKS)) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_network();
    } else {
      err = nvs_get_blob(nvs_handle, "network", network, &required_size);
    }

    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done Network");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "Network config not initialized yet!");
        set_default_network();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Read Services */
    ESP_LOGI(TAG, "Reading Service ...");
    required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "service", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size != (sizeof(services_t))) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_service();
    } else {
      err = nvs_get_blob(nvs_handle, "service", &service, &required_size);
    }
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done Services");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "Service config not initialized yet!");
        set_default_service();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Read ThingsBoard */
    ESP_LOGI(TAG, "Reading ThingsBoard ...");
    required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "thingsboard", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size != (sizeof(thingsboard_t))) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_thingsboard();
    } else {
      err = nvs_get_blob(nvs_handle, "thingsboard", &thingsboard, &required_size);
    }
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done ThingsBoard");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "ThingsBoard config not initialized yet!");
        set_default_thingsboard();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Read Leds */
    ESP_LOGI(TAG, "Reading Led ...");
    required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "led", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size == (sizeof(led_t) * MAX_LED_CHANNELS)) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_led();
    } else {
      err = nvs_get_blob(nvs_handle, "led", led, &required_size);
    }
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done Leds");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "Led config not initialized yet!");
        set_default_led();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Check leds */
    for (int i = 0; i < MAX_LED_CHANNELS; ++i) {
      if (led->version != LED_SETTINGS_VERSION) {
        set_default_led();
        break;
      }
    }

    /* Read Schedule */
    ESP_LOGI(TAG, "Reading Schedule ...");
    required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "schedule", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size == (sizeof(schedule_t) * MAX_SCHEDULE)) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_schedule();
    } else {
      err = nvs_get_blob(nvs_handle, "schedule", schedule, &required_size);
    }
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done Schedule");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "Schedule config not initialized yet!");
        set_default_schedule();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Read Schedule Config */
    ESP_LOGI(TAG, "Reading Schedule Config...");
    required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "schedule_config", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size == (sizeof(schedule_config_t))) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_schedule_config();
    } else {
      err = nvs_get_blob(nvs_handle, "schedule_config", &schedule_config, &required_size);
    }
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done Schedule config");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "Schedule config config not initialized yet!");
        set_default_schedule_config();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Check Schedule Config */
    if (schedule_config.version != SCHEDULE_CONFIG_SETTINGS_VERSION) {
      set_default_schedule_config();
    }

    /* Read Cooling */
    ESP_LOGI(TAG, "Reading Cooling...");
    required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "cooling", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size == (sizeof(cooling_t))) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_cooling();
    } else {
      err = nvs_get_blob(nvs_handle, "cooling", &cooling, &required_size);
    }
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done Cooling config");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "Cooling config config not initialized yet!");
        set_default_cooling();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Read Board GPIO config */
    ESP_LOGI(TAG, "Reading GPIO Config...");
    required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "gpio_config", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size == (sizeof(board_gpio_config_t) * MAX_BOARD_GPIO)) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_board_gpio_config();
    } else {
      err = nvs_get_blob(nvs_handle, "gpio_config", &board_gpio_config, &required_size);
    }
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done Board GPIO config");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "Board GPIO config config not initialized yet!");
        set_default_board_gpio_config();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Read Auth */
    ESP_LOGI(TAG, "Reading Auth...");
    required_size = 0;  /* value will default to 0, if not set yet in NVS */
    err = nvs_get_blob(nvs_handle, "auth", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND && required_size == (sizeof(auth_t))) {
      ESP_LOGE(TAG, "config not saved yet! error: %s", esp_err_to_name(err));
      set_default_auth();
    } else {
      err = nvs_get_blob(nvs_handle, "auth", &auth, &required_size);
    }
    switch (err) {
      case ESP_OK:
        ESP_LOGI(TAG, "Done Auth config");
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGE(TAG, "Auth config not initialized yet!");
        set_default_auth();
        break;
      default :
        ESP_LOGE(TAG, "Error (%s) reading!\n", esp_err_to_name(err));
    }

    /* Close */
    nvs_close(nvs_handle);
  }
}

void set_default_network()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  for (int i = 0; i < MAX_NETWORKS; ++i) {
    strlcpy(network[i].ssid, empty_str, 32);
    strlcpy(network[i].password, empty_str, 64);

    network[i].ip_address[0] = 192;
    network[i].ip_address[1] = 168;
    network[i].ip_address[2] = 1;
    network[i].ip_address[3] = 100;

    network[i].mask[0]       = 255;
    network[i].mask[1]       = 255;
    network[i].mask[2]       = 255;
    network[i].mask[3]       = 0;

    network[i].gateway[0]    = 192;
    network[i].gateway[1]    = 168;
    network[i].gateway[2]    = 1;
    network[i].gateway[3]    = 1;

    network[i].dns[0]        = 192;
    network[i].dns[1]        = 168;
    network[i].dns[2]        = 1;
    network[i].dns[3]        = 1;

    network[i].dhcp          = true;
    network[i].active        = false; /* hide config in web ui */

  }

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Network Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "network", network, (size_t) ((sizeof(network_t) * MAX_NETWORKS)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Network Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default Network");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_default_service()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  strlcpy(service.hostname, "ledcontroller", 32);

  /* OTA */
  strlcpy(service.ota_url, CONFIG_OTA_URL, 64);

  /* NTP */
  strlcpy(service.ntp_server, "es.pool.ntp.org", 32);
  service.utc_offset = 1;
  service.ntp_dst = false;
  service.enable_ntp = false;

  /*MQTT */
  strlcpy(service.mqtt_user, empty_str, 16);
  strlcpy(service.mqtt_password, empty_str, 16);
  service.mqtt_port = 1883;
  service.enable_mqtt = false;
  service.mqtt_qos = 0;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Service Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "service", &service, (size_t) (sizeof(services_t)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Service Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default Service");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_default_thingsboard()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* ThingsBoard Endpoint */
  strlcpy(thingsboard.endpoint, empty_str, 64);

  /* ThingsBoard MQTT */
  strlcpy(thingsboard.token, empty_str, 32);
  thingsboard.qos = 0;
  thingsboard.retain = false;
  thingsboard.rpc = false;
  thingsboard.enable = false;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Service Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "thingsboard", &thingsboard, (size_t) (sizeof(thingsboard_t)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Service Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default Service");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_default_led()
{
  esp_err_t err;
  nvs_handle nvs_handle;

#if MAX_LED_CHANNELS == 3
  const char default_colors[MAX_LED_CHANNELS][8] = {
      {"#8A7AD4"}, {"#7C9CFF"}, {"#42B8F3"},
  };
#elif MAX_LED_CHANNELS == 5
  const char default_colors[MAX_LED_CHANNELS][8] = {
      {"#8A7AD4"}, {"#7C9CFF"}, {"#42B8F3"}, {"#4DF7FF"}, {"#6EB96E"}
  };
#else
  const char default_colors[MAX_LED_CHANNELS][8] = {
      {"#8A7AD4"}, {"#7C9CFF"}, {"#42B8F3"}, {"#4DF7FF"}, {"#6EB96E"}, {"#FDFE90"}, {"#FB647A"}, {"#990000"}
  };
#endif

  for(size_t i = 0; i < MAX_LED_CHANNELS; i++)
  {
    led[i].id    = i;
    led[i].power = 0;                             /* 0 ->  0->0 in Watts x 10 */
    led[i].state = 1;                             /* 0 ->  OFF | 1 -> ON */
    led[i].sync_channel = 0;
    led[i].sync_channel_group = i;
    led[i].duty_max = 255;
    led[i].version = LED_SETTINGS_VERSION;
    strlcpy(led[i].color, default_colors[i], 8);  /* default color #B4D9F1 -> 'Cold White' in UI */
  }

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Led Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "led", led, (size_t) ((sizeof(led_t) * MAX_LED_CHANNELS)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Led Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default Led");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_default_schedule()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  for(size_t i = 0; i < MAX_SCHEDULE; i++)
  {
    schedule[i].time_hour       = 0;
    schedule[i].time_minute     = 0;
    schedule[i].duty[0]         = 0; /* CH1 Default */
    schedule[i].duty[1]         = 0; /* CH2 Default */
    schedule[i].duty[2]         = 0; /* CH3 Default */

#if MAX_LED_CHANNELS >= 4
    schedule[i].duty[3]         = 0; /* CH4 Default */
#endif
#if MAX_LED_CHANNELS >= 5
    schedule[i].duty[4]         = 0; /* CH5 Default */
#endif
#if MAX_LED_CHANNELS == 8
    schedule[i].duty[5]         = 0; /* CH6 Default */
    schedule[i].duty[6]         = 0; /* CH7 Default */
    schedule[i].duty[7]         = 0; /* CH8 Default */
#endif
    schedule[i].brightness      = 0; /* All Channels brightness */
    schedule[i].active          = false;
  }

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Schedule Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "schedule", schedule, (size_t) ((sizeof(schedule_t) * MAX_SCHEDULE)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Schedule Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default Schedule");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_default_schedule_config()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  schedule_config.mode                  = ADVANCED;
  schedule_config.sunrise_hour          = 0;
  schedule_config.sunrise_minute        = 0;
  schedule_config.sunset_hour           = 0;
  schedule_config.sunset_minute         = 0;
  schedule_config.simple_mode_duration  = 30;
  schedule_config.brightness            = 0;
  schedule_config.rgb                   = false;
  schedule_config.gamma                 = 100; /* gamma correction x100 */
  schedule_config.use_sync              = 1;
  schedule_config.sync_group            = 0;
  schedule_config.sync_master           = 0;
  schedule_config.version               = SCHEDULE_CONFIG_SETTINGS_VERSION;

  for(size_t i = 0; i < MAX_LED_CHANNELS; i++)
  {
    schedule_config.duty[i] = 0;
  }

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Schedule cfg Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "schedule_config", &schedule_config, sizeof(schedule_config_t));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Schedule cfg Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default Schedule cfg");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_default_cooling()
{
  esp_err_t err;
  nvs_handle nvs_handle;

#ifdef USE_FAN_PWM
  cooling.installed   = true;
#else
  cooling.installed   = false;
#endif
  cooling.start_temp  = 35;
  cooling.target_temp = 50;
  cooling.max_temp    = 70;
  cooling.pid_kp      = 0.1;
  cooling.pid_ki      = 0.4;
  cooling.pid_kd      = 0.01;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Cooling in NVS ...");
    err = nvs_set_blob(nvs_handle, "cooling", &cooling, sizeof(cooling_t));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Cooling in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default Cooling");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_default_board_gpio_config()
{
  esp_err_t err;
  nvs_handle nvs_handle;
  for (int i = 0; i < MAX_BOARD_GPIO; ++i) {
    board_gpio_config[i].pin = board_gpio_map[i];
    board_gpio_config[i].function = NA;
    board_gpio_config[i].alt_function = NA;
  }

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating GPIO Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "gpio_config", &board_gpio_config,
                       sizeof(board_gpio_config_t) * MAX_BOARD_GPIO);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update GPIO Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default GPIO Config");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_default_auth()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  strlcpy(auth.user, "admin", 32);
  strlcpy(auth.password, "12345678", 32);

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Auth in NVS ...");
    err = nvs_set_blob(nvs_handle, "auth", &auth, sizeof(auth_t));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Auth in NVS");
    } else {
      ESP_LOGI(TAG, "Done Default Auth");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_network(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Network Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "network", network, (size_t) ((sizeof(network_t) * MAX_NETWORKS)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Network Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Network");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_service(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Service Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "service", &service, (size_t) (sizeof(services_t)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Service Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Service");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_thingsboard(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating ThingsBoard Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "thingsboard", &thingsboard, (size_t) (sizeof(thingsboard_t)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update ThingsBoard Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New ThingsBoard");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_led(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Led Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "led", led, (size_t) ((sizeof(led_t) * MAX_LED_CHANNELS)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Led Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Led");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_schedule(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Schedule Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "schedule", schedule, (size_t) ((sizeof(schedule_t) * MAX_SCHEDULE)));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Schedule Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Schedule");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_schedule_config(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Schedule cfg Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "schedule_config", &schedule_config, sizeof(schedule_config_t));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Schedule Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Schedule Config");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_cooling(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Cooling in NVS ...");
    err = nvs_set_blob(nvs_handle, "cooling", &cooling, sizeof(cooling_t));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Cooling in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Cooling");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_board_gpio_config(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating GPIO Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "gpio_config", &board_gpio_config,
                       sizeof(board_gpio_config_t) * MAX_BOARD_GPIO);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update GPIO Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New GPIO Config");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void set_auth(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write */
    ESP_LOGI(TAG, "Updating Auth in NVS ...");
    err = nvs_set_blob(nvs_handle, "auth", &auth, sizeof(auth_t));
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to Update Auth in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Auth");
    }
  }

  ESP_LOGI(TAG, "Committing updates in NVS ...");
  err = nvs_commit(nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to commit NVS");
  } else {
    ESP_LOGI(TAG, "Done commit NVS");
  }

  /* Close */
  nvs_close(nvs_handle);
};

void erase_settings(void)
{
  ESP_ERROR_CHECK(nvs_flash_erase());
}

network_t * get_networks(uint8_t network_id)
{
  if (network_id >= MAX_NETWORKS)
    return &network[0];

  return &network[network_id];
}

services_t * get_services(void)
{
  return &service;
}

thingsboard_t * get_thingsboard(void)
{
  return &thingsboard;
}


led_t * get_leds(uint8_t led_id)
{
  if (led_id >= MAX_LED_CHANNELS)
    return &led[0];

  return &led[led_id];
}

schedule_t * get_schedule(uint8_t schedule_id)
{
  if (schedule_id >= MAX_SCHEDULE)
    return &schedule[0];

  return &schedule[schedule_id];
}

schedule_config_t * get_schedule_config(void)
{
  return &schedule_config;
}

cooling_t * get_cooling(void)
{
  return &cooling;
}

board_gpio_config_t * get_board_gpio_config(uint8_t id)
{
  if (id >= MAX_BOARD_GPIO)
    return &board_gpio_config[0];

  return &board_gpio_config[id];
}

auth_t * get_auth(void)
{
  return &auth;
}

void ip_to_string(uint8_t ip[4], char* string)
{
  snprintf(string, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void string_to_ip(const char *ip_string, uint8_t *octets)
{
  char * octet;
  char ip_address[16];

  memset(ip_address, 0, 16);
  strcpy(ip_address, ip_string);

  octet = strtok(ip_address, ".");
  for (int j = 0; j < 4; ++j) {

    octets[j] = (uint8_t) atoi(octet);
    octet = strtok(NULL, ".");
  }
}