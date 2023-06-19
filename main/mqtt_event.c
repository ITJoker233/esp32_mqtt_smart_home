#include "mqtt_event.h"

char *MATT_TAG = "MQTT";

esp_mqtt_client_handle_t client;
char publick_id[64];
unsigned char count_time;

static esp_err_t mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        char public[60];
        ESP_LOGI(MATT_TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, publick_id, 1);
        ESP_LOGI(MATT_TAG, "sent subscribe successful, msg_id=%d", msg_id);
        ESP_LOGI(MATT_TAG, "sent subscribe successful, publick_id=%s", publick_id);
        ESP_LOGI(MATT_TAG, "%s online", publick_id);
        sprintf(public, PUBLICK_NAME, "common");
        // esp_mqtt_client_publish(client, public, MIDDLE_ONLINE_PACKET(), 0, 1, 0); // 公共频道
        esp_mqtt_client_publish(client, publick_id, "{\"status\":1}", 0, 1, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MATT_TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MATT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MATT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        break;
    case MQTT_EVENT_DATA:
        // 首先整体判断是否为一个json格式的数据
        cJSON *pJsonRoot = cJSON_Parse(event->data);
        ESP_LOGI(MATT_TAG, "接受到mqtt数据");
        if (pJsonRoot != NULL)
        {
            /// 收到服务器端的数据
            printf("%s\n", cJSON_Print(pJsonRoot)); // 打包成功调用cJSON_Print打印输出
            protocol_parse(pJsonRoot);
        }
        cJSON_Delete(pJsonRoot);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MATT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            ESP_LOGI(MATT_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }

        // esp_mqtt_client_start(client);
        esp_restart();
        // esp_mqtt_client_reconnect();
        break;
    default:
        ESP_LOGI(MATT_TAG, "Other event id:%d", event->event_id);
        // esp_mqtt_client_start(client);
        // esp_restart();
        break;
    }
    return ESP_OK;
}
void mqtt_service_start(void)
{
    char client_id[64];
    sprintf(client_id, "ESP32-%s", publick_id);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.hostname = mqtt_host,
        .broker.address.port = mqtt_port,
        .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
        .credentials.client_id = client_id,
        .credentials.username = mqtt_username,
        .credentials.authentication.password = mqtt_password,
        .session.keepalive = 30,
        .session.disable_clean_session = false,
        .session.last_will.topic = publick_id,
        .session.last_will.msg = "{\"status\":0}",
        .session.last_will.qos = 1,
        .session.last_will.retain = 1,
        .buffer.size = 2048,
        .buffer.out_size = 2048,
        .task.stack_size = 4096,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
void mqtt_send_task(void)
{
    static uint16_t count;
    count = 0;
    while (1)
    {
        count++;
        if (count >= 10 * MQTT_WORK_INTERVAL) // 正常工作包
        {
            count = 0;
            // esp_mqtt_client_publish(client, publick_id, WORK_PACKET(), 0, 0, 0);
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}