#ifndef DS18B20_BITBANG_H
#define DS18B20_BITBANG_H

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief Khởi tạo chân DQ cho DS18B20
 * 
 * @param gpio_num Chân GPIO dùng làm DQ (dữ liệu 1-Wire)
 * @return esp_err_t ESP_OK nếu ok, lỗi nếu không thể config
 */
esp_err_t ds18b20_bb_init(gpio_num_t gpio_num);

/**
 * @brief Đọc nhiệt độ từ DS18B20 (°C)
 * 
 * @param temperature con trỏ để nhận nhiệt độ
 * @return esp_err_t ESP_OK nếu đọc thành công, lỗi nếu không (reset gagal, CRC sai...)
 */
esp_err_t ds18b20_bb_read_temp(float *temperature);

#endif  // DS18B20_BITBANG_H
