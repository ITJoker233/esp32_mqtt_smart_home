#include "protocol.h"
#include "utils.h"
DEVICE_DATA device_data;
char *PROTOCOL_TAG = "Protocol";

void RESPONSE_PACKET(uint8_t status_code, cJSON *array)
{

    cJSON *response_packet = cJSON_CreateObject();
    uint32_t timestamp; // 13位时间，精确到ms
    time_t timer0;
    timestamp = time(NULL);
    cJSON_AddStringToObject(response_packet, "name", device_data.device_name); // 设备名称
    cJSON_AddStringToObject(response_packet, "type", device_data.device_type); // 设备类型
    cJSON_AddStringToObject(response_packet, "mac", device_data.mac);          // mac 地址
    cJSON_AddStringToObject(response_packet, "role", "device");                // 身份

    cJSON_AddNumberToObject(response_packet, "timestamp", timestamp); // 时间戳
    cJSON_AddNumberToObject(response_packet, "code", status_code);    // 响应状态码
    cJSON_AddNumberToObject(response_packet, "status", 1);            //
    cJSON_AddItemToArray(array, response_packet);
}
char *LOGIN_PACKET(void) // 登录包
{
    cJSON *login_packet = cJSON_CreateObject();

    cJSON_AddStringToObject(login_packet, "master", device_data.master); // mac 地址
    RESPONSE_PACKET(200, login_packet);

    char *json_data = cJSON_Print(login_packet); // JSON数据结构转换为JSON字符串

    cJSON_free((void *)json_data);
    cJSON_Delete(login_packet); // 清除结构体
    return json_data;
}
char *HEARTBEAT_PACKET(void) // 心跳包
{
    static uint32_t clk = 0;

    cJSON *heartbeat_packet = cJSON_CreateObject();
    uint32_t msize = esp_get_free_heap_size();
    char timestamp[64]; // 13位时间，精确到ms
    time_t timer0;
    timer0 = time(NULL);
    sprintf(timestamp, "%lld%d", timer0, get_sys_time_ms());
    cJSON_AddNumberToObject(heartbeat_packet, "clk", clk); // clk
    ESP_LOGI(PROTOCOL_TAG, "clk = %d,%d\r\n", clk, msize);
    RESPONSE_PACKET(200, heartbeat_packet);

    char *json_data = cJSON_Print(heartbeat_packet); // JSON数据结构转换为JSON字符串

    // memcpy(mqtt_send_data, json_data, sizeof(json_data));
    cJSON_free((void *)json_data);
    cJSON_Delete(heartbeat_packet); // 清除结构体
    return json_data;
}

void protocol_init(void)
{

    memset(&device_data, 0, sizeof(device_data));
    get_device_mac(device_data.mac);
    ESP_LOGI(PROTOCOL_TAG, "Init Success!");
}

int cjson_string_cmp(cJSON *cjson_item, char *key, char *data)
{
    if (!cjson_item)
        return 0;
    cJSON *cjs = cJSON_GetObjectItem(cjson_item, key);
    if (!cjs)
        return 0;
    else
    {
        char da[30] = {0};
        strcpy(da, cjs->valuestring);
        if (strcmp(da, data) == 0)
            return 1;
        else
            return 0;
    }
}
void protocol_setting_packet(cJSON *cjson)
{

    char *sid = cJSON_GetObjectItem(cjson, "sid")->valuestring;
    char nvs_sid[32] = "";
    size_t sid_size = sizeof(nvs_sid);
    esp_err_t sid_err = from_nvs_get_value("sid", nvs_sid, &sid_size);
    if (strlen(nvs_sid) == 0)
    {
        from_nvs_set_value("sid", sid); // 添加安全码
        ESP_LOGI(PROTOCOL_TAG, "safe code setting success!\r\n");
    }
    else
    {
        ESP_LOGI(PROTOCOL_TAG, "safe code setting fail!\r\n");
    }
}

void protocol_control_packet(cJSON *cjson)
{
    mbedtls_md5_context md5_ctx;
    unsigned char sign_md5[64];
    char sign_md5_str[16];

    char str_sid[32];
    size_t size = sizeof(str_sid);
    esp_err_t err = from_nvs_get_value("sid", str_sid, &size); /*   取NVS里的sid*/
    if (strlen(str_sid) == 0)
    {
        ESP_LOGI("sid", "sid is null!");
        return;
    }
    char *sign = cJSON_GetObjectItem(cjson, "sign")->valuestring;
    int mid_sid_length = strlen("") + strlen(str_sid);
    char sign_encrypt[255];
    memset(sign_encrypt, '\0', mid_sid_length);
    strcpy(sign_encrypt, "");
    strcat(sign_encrypt, str_sid);

    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
    mbedtls_md5_update(&md5_ctx, (unsigned char *)sign_encrypt, strlen(sign_encrypt));
    mbedtls_md5_finish(&md5_ctx, sign_md5);
    mbedtls_md5_free(&md5_ctx);

    hex_to_str(sign_md5_str, sign_md5, 32);
    for (int i = 0; i < 32; i++)
    {
        if (sign[i] != sign_md5_str[i])
        {
            ESP_LOGI("Sign", "sign error!");
            return;
        }
    }
    ESP_LOGI("Sign", "sign success!");
}

void protocol_parse(cJSON *cjson)
{

    // 收到服务器端的数据
    if (cjson_string_cmp(cjson, "role", "system"))
        ;
    else
    {
        return;
    }
    // 判断是否存在Key
    if (cJSON_HasObjectItem(cjson, "role") == 0)
    {
        ESP_LOGI(PROTOCOL_TAG, "check faild!\r\n");
        return;
    }
}