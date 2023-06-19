#ifndef __SNTP_EVENT_H__
#define __SNTP_EVENT_H__

#include "common.h"
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

void sntp_start(void);
int get_sys_time_ms(void);
#endif