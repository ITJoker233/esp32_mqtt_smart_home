#ifndef __WIFI_EVENT_H__
#define __WIFI_EVENT_H__
#include "common.h"

httpd_handle_t start_webserver(void);

void wifi_init_sta();
void wifi_init_softap(void);
void wifi_station_task(void *pvParameters);

#endif