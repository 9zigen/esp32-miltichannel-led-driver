set(SOURCES
    "thingsboard.c"
    "auth.c"
    "pwm.c"
    "fanspeed.c"
    "pidreg.c"
    "adc.c"
    "light.c"
    "rtc.c"
    "main.c"
    "server.c"
    "main.c"
    "server.c"
    "monitor.c"
    "mcp7940.c"
    "connect.c"
    "rtc.c"
    "light.c"
    "led.c"
    "mqtt.c"
    "buttons.c"
    "thingsboard.c"
    "udp_multicast.c"
    "tools.c"
    "web_socket.c")

idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS . include
    EMBED_TXTFILES ../frontend/public/favicon.ico
    REQUIRES esp_netif esp_http_server ota captive app_settings app_mqtt_discovery app_mqtt app_events json driver esp_timer esp_wifi esp_adc)

##set(COMPONENT_EMBED_TXTFILES ../certs/io_alab_cc.pem ../certs/mqtt_io_alab_cc.pem)
#set(COMPONENT_EMBED_TXTFILES ../frontend/public/favicon.ico)
#register_component()
