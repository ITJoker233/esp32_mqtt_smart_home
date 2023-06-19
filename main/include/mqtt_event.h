#ifndef __MQTT_EVENT_H__
#define __MQTT_EVENT_H__
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "cJSON.h"
#include "protocol.h"
#include "sntp_event.h"
#include "common.h"
#define mqtt_port 1883
#define mqtt_host "127.0.0.1"
#define mqtt_username "admin"
#define mqtt_password "123456"
void mqtt_service_start(void);
void mqtt_send_task(void);
#endif