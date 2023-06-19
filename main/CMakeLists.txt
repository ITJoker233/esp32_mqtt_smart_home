idf_component_register(SRCS "sntp_event.c" "protocol.c" "wifi_event.c" "gpio_event.c" "mqtt_event.c" "main.c" "dns_event.c" "utils.c"
                    INCLUDE_DIRS "include"
                    EMBED_FILES root.html)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
