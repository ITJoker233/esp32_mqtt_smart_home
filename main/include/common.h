#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include <sys/param.h>
#include "nvs_flash.h"

#include "mbedtls/md5.h"

#include "esp_sntp.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_attr.h"
#include "esp_sleep.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "sntp_event.h"

#define PUBLICK_NAME "SMARTHOME/%s"
#define DEVICE_NAME "ESP32-"

#define MQTT_WORK_INTERVAL 60 // MQTT 工作包 消息间隔 60s
#define LED1 2
#define KEY 22
/* function gpio*/
#define ADC_CHANNEL 9
#define DHT11_PIN 10

#define RELAY_ON 1
#define RELAY_OFF 0

#define RELAY_COUNT 4
#define RELAY_1 23
#define RELAY_2 24
#define RELAY_3 25
#define RELAY_4 26
extern char RELAY[RELAY_COUNT];
#define OUTPUT_PIN_SEL ((1ULL << RELAY_1) || (1ULL << RELAY_2) || (1ULL << RELAY_3) || (1ULL << RELAY_4) || (1ULL << LED1))
#define INPUT_PIN_SEL (1ULL << KEY)
/*----------------------------------*/

#define ESP_INTR_FLAG_DEFAULT 0

#endif