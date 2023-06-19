#include "gpio_event.h"
#include "utils.h"
#include "protocol.h"
char *GPIO_TAG = "gpio";
char RELAY[RELAY_COUNT] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4};

static void relay_task(void)
{
    static uint32_t relay_timestamp_tmp;
    static uint32_t relay_time_tick = 0;
    for (;;)
    {
        for (int i = 0; i < RELAY_COUNT; i++)
        {
            relay_timestamp_tmp = time(NULL);
            if (relay_timestamp_tmp >= device_data.relay_timestamp[i]) // 当实际值大于预设值时
            {
                device_data.relay_timestamp[i] = 0;
                if (device_data.relay_timestamp_value[i] == 1 || device_data.relay_timestamp_value[i] == 0) // 预防异常值
                {
                    device_data.relay_value[i] = device_data.relay_timestamp_value[i];
                    device_data.relay_timestamp_value[i] = -1;
                }
            }
            gpio_set_level(RELAY[i], device_data.relay_value[i]);
            ESP_LOGI(GPIO_TAG, "tick: %d RELAY[%d] Value: %d\r\n", relay_time_tick, i + 1, device_data.relay_value[i]);
        }
        relay_time_tick++;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static void esp32_reset_task(void)
{
    int num = 0;
    uint8_t cnt = 0;
    gpio_set_level(LED1, 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    for (;;)
    {
        if (gpio_get_level(KEY) == 0) // 按键按下
        {
            gpio_set_level(LED1, 1);

            num++;
            if (num >= 100)
            {
                gpio_set_level(LED1, 0);
                printf("按下重置...\r\n");
                ESP_ERROR_CHECK(from_nvs_set_value("wifi_ssid", ""));
                ESP_ERROR_CHECK(from_nvs_set_value("wifi_pwd", ""));
                gpio_set_level(LED1, 1); // 绿灯
                esp_restart();
            }
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}
void gpio_ap_blink(void)
{
    while (1)
    {
        gpio_set_level(LED1, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED1, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
void gpio_init(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO2/15
    io_conf.pin_bit_mask = OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // bit mask of the pins, use GPIO22 here
    io_conf.pin_bit_mask = INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    gpio_set_level(LED1, 0);
    // start gpio task
    xTaskCreate(esp32_reset_task, "esp32_reset_task", 2048, NULL, 10, NULL);
}
