#ifndef TFT_H
#define TFT_H
#pragma once
#include <stdint.h>

void tft_init(void);
void tft_display_data(float temp, float ph, float tds, float turb);
void tft_update_task(void *pv);
#endif