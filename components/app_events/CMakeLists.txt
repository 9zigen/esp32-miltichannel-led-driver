set(SOURCES "app_events.c")
idf_component_register(SRCS ${SOURCES}
        INCLUDE_DIRS . include
        REQUIRES mqtt json)