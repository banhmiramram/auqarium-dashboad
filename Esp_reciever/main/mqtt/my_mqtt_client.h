#pragma once

#include "esp_err.h"
#include "mqtt_client.h"   // ESP-IDF MQTT client
#include "esp_log.h"
#include "cJSON.h"         // Nếu bạn dùng JSON

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi động MQTT client
 */
void mqtt_app_start(void);

/**
 * @brief Publish dữ liệu cảm biến dưới dạng JSON
 *
 * @param temp  Nhiệt độ
 * @param ph    Độ pH
 * @param tds   Nồng độ TDS
 * @param turb  Độ đục
 */
void mqtt_publish_data(float temp, float ph, float tds, float turb);

#ifdef __cplusplus
}
#endif
