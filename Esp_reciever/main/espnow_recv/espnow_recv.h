#ifndef ESPNOW_RECV_H
#define ESPNOW_RECV_H

#include <stdbool.h>
#include "esp_err.h"

void add_warning(const char *type, const char *msg, int level);
void espnow_init(void);
void check_sensor_levels(void);
void espnow_task(void *pv);

#endif
