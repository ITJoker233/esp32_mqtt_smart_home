#ifndef __UTILS_H__
#define __UTILS_H__
void get_device_mac(char *mac);
esp_err_t from_nvs_set_value(char *key, char *value);
esp_err_t from_nvs_get_value(char *key, char *value, size_t *size);
void hex_to_str(char *ptr, unsigned char *buf, int len);

#endif