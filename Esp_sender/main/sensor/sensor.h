#ifndef SENSOR_H
#define SENSOR_H

#include "esp_err.h"

#define PH_PIN    ADC_CHANNEL_7   // GPIO35
#define TDS_PIN   ADC_CHANNEL_4   // GPIO32
#define TURB_PIN  ADC_CHANNEL_6   // GPIO34


esp_err_t sensor_init(void);

float sensor_read_ph(void);
float sensor_read_tds(void);
float sensor_read_turb(void);

#endif
