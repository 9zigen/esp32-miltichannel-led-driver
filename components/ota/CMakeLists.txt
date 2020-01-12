set(COMPONENT_SRCS 	"ota.c")
set(COMPONENT_ADD_INCLUDEDIRS "./include")
set(COMPONENT_REQUIRES esp_http_client esp_https_ota nvs_flash app_update)
register_component()