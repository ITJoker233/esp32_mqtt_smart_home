#include "common.h"
const char *UTILS_TAG = "Common";

void hex_to_str(char *ptr, unsigned char *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        sprintf(ptr, "%02X", buf[i]);
        ptr += 2;
    }
}

void get_device_mac(char *mac)
{
    uint8_t device_mac[6];
    esp_efuse_mac_get_default(device_mac);
    hex_to_str(mac, device_mac, 6);
}

//---------------------nvs-----------------------//
void nvs_get_str_log(esp_err_t err, char *key, char *value)
{
    switch (err)
    {
    case ESP_OK:
        // ESP_LOGI(UTILS_TAG, "%s = %s", key, value);
        ESP_LOGI(UTILS_TAG, "nvs get %s value success!", key);
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGI(UTILS_TAG, "%s : Can't find in NVS!", key);
        break;
    default:
        ESP_LOGE(UTILS_TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
}

esp_err_t from_nvs_set_value(char *key, char *value)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("memory", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(UTILS_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        err = nvs_set_str(my_handle, key, value);
        ESP_LOGI(UTILS_TAG, "set %s is %s!,the err is %d\n", key, (err == ESP_OK) ? "succeed" : "failed", err);
        nvs_close(my_handle);
        ESP_LOGI(UTILS_TAG, "NVS close Done\n");
    }
    return ESP_OK;
}

esp_err_t from_nvs_get_value(char *key, char *value, size_t *size)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("memory", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(UTILS_TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }
    else
    {
        err = nvs_get_str(my_handle, key, value, size);
        nvs_get_str_log(err, key, value);
        nvs_close(my_handle);
    }
    return err;
}