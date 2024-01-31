set(SOURCES "app_settings.c")
idf_component_register(SRCS ${SOURCES}
        INCLUDE_DIRS . include
        REQUIRES nvs_flash driver)