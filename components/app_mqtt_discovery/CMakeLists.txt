set(SOURCES
        "app_mqtt_discovery.c"
        "app_mqtt_discovery_sensors.c")

idf_component_register(SRCS ${SOURCES}
        INCLUDE_DIRS . include
        REQUIRES app_settings app_mqtt json)