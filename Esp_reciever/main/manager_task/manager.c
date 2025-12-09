#include "global/global.h"
#include "esp_log.h"
#include "manager.h"

#define CONFIG_FREERTOS_HZ 100
static const char *TAG = "MANAGER";

void manager_task(void *pv)
{
    sensor_data_t s;

    ESP_LOGI(TAG, "Manager task started on core %d", xPortGetCoreID());

    while (1) {
        if (xQueueReceive(xEspNowQueue, &s, portMAX_DELAY) == pdTRUE) {

            // Cập nhật biến global an toàn
            if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                g_temp = s.temp;
                g_ph   = s.ph;
                g_tds  = s.tds;
                g_turb = s.turb;
                new_data = true;
                xSemaphoreGive(data_mutex);
            }

            // Gửi sang WiFi task (nếu queue đầy thì bỏ)
            if (xWifiQueue) {
                if (xQueueSend(xWifiQueue, &s, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "xWifiQueue full - drop data");
                }
            }

            ESP_LOGI(TAG, "Data updated: T=%.2f pH=%.2f", s.temp, s.ph);
        }
    }
}
