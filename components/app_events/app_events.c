//
// Created by Aleksey Volkov on 2/10/22.
//

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_event.h"
#include "esp_log.h"

#include "app_events.h"

static const char *TAG="EVENT BUS";

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(APP_EVENTS);
ESP_EVENT_DEFINE_BASE(APP_MQTT_EVENTS);

esp_event_loop_handle_t app_event_loop;

static void application_event_task(void* args)
{

}

/* ToDO: implement events handling */
static void app_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
  ESP_LOGD(TAG, "handling %s:%ld from", event_base, event_id);
  if (event_id == LIGHT_CHANGE_EVENT) {
//    mqtt_publish_channel_state();
//    mqtt_publish_channel_duty();
//    mqtt_publish_brightness();
  }
}

esp_err_t init_events()
{
  esp_event_loop_args_t app_loop_args = {
      .queue_size = 10,
      .task_name = "app_events",
      .task_priority = 5,
      .task_stack_size = 2048,
      .task_core_id = 1
  };

  /* Create the event loops */
  ESP_ERROR_CHECK(esp_event_loop_create(&app_loop_args, &app_event_loop));

  /* Register the handler */
  ESP_ERROR_CHECK(esp_event_handler_instance_register_with(app_event_loop, APP_EVENTS, LIGHT_CHANGE_EVENT, app_event_handler, app_event_loop, NULL));

  return ESP_OK;
}

void app_events_register_handler(app_event_t event, void* arg, app_event_handler_t handler, esp_event_handler_instance_t ctx)
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(app_event_loop,
                                                             APP_EVENTS,
                                                             event,
                                                             handler,
                                                             app_event_loop,
                                                             ctx));
}

void notify_app(app_event_t event, const void* event_data, size_t event_data_size)
{
  ESP_ERROR_CHECK(esp_event_post_to(app_event_loop, APP_EVENTS, event, event_data, event_data_size, portMAX_DELAY));
}