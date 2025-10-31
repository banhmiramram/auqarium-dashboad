#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include <stdbool.h>

extern bool wifi_connected;  // để main kiểm tra trạng thái

void wifi_init_apsta(void);

#endif
