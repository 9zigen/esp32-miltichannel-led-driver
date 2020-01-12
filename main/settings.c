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
#include "settings.h"

static const char *TAG = "SETTINGS";

network_t network[MAX_NETWORKS];
services_t service;
led_t led[MAX_LED_CHANNELS];
schedule_t schedule[MAX_SCHEDULE];

const char * empty_str = " ";

/* Initialize Settings */
void init_settings()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  ESP_LOGE(TAG, "calloc settings structs in memory");

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

    /* Read Networks Config */
    ESP_LOGI(TAG, "Reading Networks Config ...");
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

    /* Read Services Config */
    ESP_LOGI(TAG, "Reading Service Config ...");
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

    /* Read Leds Config */
    ESP_LOGI(TAG, "Reading Led Config ...");
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

    /* Read Schedule Config */
    ESP_LOGI(TAG, "Reading Schedule Config ...");
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

  char hostname_buf[20];
  sprintf(hostname_buf, "LED_%d", esp_random());
  strlcpy(service.hostname, hostname_buf, 20);

  /* NTP */
  strlcpy(service.ntp_server, "es.pool.ntp.org", 20);
  service.utc_offset = 1;
  service.ntp_dst = false;
  service.enable_ntp = true;

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

void set_default_led()
{
  esp_err_t err;
  nvs_handle nvs_handle;

  const char default_colors[MAX_LED_CHANNELS][8] = {
      {"#8A7AD4"}, {"#7C9CFF"}, {"#42B8F3"}, {"#4DF7FF"}, {"#6EB96E"}, {"#FDFE90"}, {"#FB647A"}, {"#990000"}};

  for(size_t i = 0; i < MAX_LED_CHANNELS; i++)
  {
    led[i].id    = i; 
    led[i].power = 0;                             /* 0 ->  0->0 in Watts x 10 */
    led[i].state = 1;                             /* 0 ->  OFF | 1 -> ON */
    strlcpy(led[i].color, default_colors[i], 8);  /* default color #DDEFFF -> 'Cold White' in UI */
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
    schedule[i].duty[3]         = 0; /* CH4 Default */
    schedule[i].duty[4]         = 0; /* CH5 Default */
    schedule[i].duty[5]         = 0; /* CH6 Default */
    schedule[i].duty[6]         = 0; /* CH7 Default */
    schedule[i].duty[7]         = 0; /* CH8 Default */
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

void set_all_settings(void)
{
  esp_err_t err;
  nvs_handle nvs_handle;

  /* Open Storage */
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error: unable to open the NVS partition");
  } else {
    /* Write Network */
    ESP_LOGI(TAG, "Updating Network Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "network", network, (size_t) ((sizeof(network_t) * MAX_NETWORKS)));
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to Update Network Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Network");
    }

    /* Write Service */
    ESP_LOGI(TAG, "Updating Service Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "service", &service, (size_t) (sizeof(services_t)));
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to Update Service Config in NVS");
    } else {
      ESP_LOGI(TAG, "Done New Service");
    }

    /* Write Led */
    ESP_LOGI(TAG, "Updating Led Config in NVS ...");
    err = nvs_set_blob(nvs_handle, "led", led, (size_t) ((sizeof(led_t) * MAX_LED_CHANNELS)));
    if (err != ESP_OK) {
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

void erase_settings(void)
{
  ESP_ERROR_CHECK(nvs_flash_erase());
}

network_t * get_network_config(uint8_t network_id)
{
  if (network_id >= MAX_NETWORKS)
    return &network[0];

  return &network[network_id];
}

services_t * get_service_config(void)
{
  return &service;
}

led_t * get_led_config(uint8_t led_id)
{
  if (led_id >= MAX_LED_CHANNELS)
    return &led[0];

  return &led[led_id];
}

schedule_t * get_schedule_config(uint8_t schedule_id)
{
  if (schedule_id >= MAX_SCHEDULE)
    return &schedule[0];

  return &schedule[schedule_id];
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