#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "common.h"

typedef struct
{
    char device_name[64];
    char device_type[64];
    char master[64]; // 主人名称
    char adc_value[64];
    char mac[64];
    char sign[64];
    char relay_value[4];
    char relay_timestamp_value[4]; // 定时开关存的值
    uint32_t relay_timestamp[4];   // 0 默认开关， 大于0 则进行定时开关
    uint32_t start_run_time;       // 开始运行时间
} DEVICE_DATA;

extern DEVICE_DATA device_data;
void protocol_init(void);
void protocol_parse(cJSON *cjson);
void RESPONSE_PACKET(uint8_t status_code, cJSON *array);
#endif