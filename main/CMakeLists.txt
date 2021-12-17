set(SOURCES
    "thingsboard.c" "auth.c" "pwm.c" "fanspeed.c" "pidreg.c"
    "adc.c" "light.c" "mqtt.c" "rtc.c" "main.c" "server.c"
    "settings.c" "main.c" "server.c" "monitor.c" "mcp7940.c"
    "connect.c" "settings.c" "rtc.c" "light.c" "mqtt.c" "led.c"
    "buttons.c" "thingsboard.c" "udp_multicast.c" "tools.c")

idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS . include
    EMBED_TXTFILES ../frontend/public/favicon.ico
    REQUIRES esp_netif nvs_flash esp_http_server ota mqtt json mdns esp_adc_cal)

##set(COMPONENT_EMBED_TXTFILES ../certs/io_alab_cc.pem ../certs/mqtt_io_alab_cc.pem)
#set(COMPONENT_EMBED_TXTFILES ../frontend/public/favicon.ico)
#register_component()
