#ifndef OLED_H
#define OLED_H

#include "driver/i2c.h"
#include "esp_err.h"

#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_I2C_PORT I2C_NUM_0
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

esp_err_t oled_init(void);
void oled_clear(void);
void oled_show_text(const char *text, uint8_t x, uint8_t y);

#endif
