#include <sys/param.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "common.h"
#include "utils.h"
#include "protocol.h"
#include "wifi_event.h"
#include "gpio_event.h"
#include "mqtt_event.h"
#include "sntp_event.h"
#include "dns_event.h"

extern SemaphoreHandle_t ap_sem;
extern httpd_handle_t server;
TaskHandle_t wifi_station_task_handler;

void app_main(void)
{

    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    char ssid[64] = "";
    size_t ssid_size = sizeof(ssid);
    esp_err_t err = from_nvs_get_value("wifi_ssid", ssid, &ssid_size);

    gpio_init();
    protocol_init();
    if (err == ESP_OK && strlen(ssid) > 1)
    {
        wifi_init_sta();
        sntp_start();
        mqtt_send_task();
        xTaskCreate(mqtt_send_task, "mqtt_send_task", 4096, NULL, configMAX_PRIORITIES, NULL);
    }
    else // 无wifi信息
    {
        ap_sem = xSemaphoreCreateBinary();

        esp_netif_create_default_wifi_ap();

        wifi_init_softap();

        server = start_webserver();

        start_dns_server();

        xTaskCreate(gpio_ap_blink, "wifi_station_task", 4096, NULL, 4, &wifi_station_task_handler);
        xTaskCreate(wifi_station_task, "wifi_station_task", 4096, NULL, 4, &wifi_station_task_handler);
    }
}