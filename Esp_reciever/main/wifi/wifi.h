#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include <stdbool.h>

extern bool wifi_connected;

void wifi_init_apsta(void);
void wifi_task(void *pv);

#endif
