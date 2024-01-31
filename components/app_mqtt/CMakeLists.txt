set(SOURCES "app_mqtt.c")
idf_component_register(SRCS ${SOURCES}
        INCLUDE_DIRS . include
        REQUIRES mqtt json esp_app_format app_settings app_events app_mqtt_discovery)