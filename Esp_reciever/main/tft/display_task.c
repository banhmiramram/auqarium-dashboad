#include "tft/tft.h"
#include "global/global.h"
#include "esp_log.h"

#define CONFIG_FREERTOS_HZ 100

static const char *TAG = "DISPLAY";

void display_task(void *pv)
{
    ESP_LOGI(TAG, "Display task started on core %d", xPortGetCoreID());

    float temp = 0, ph = 0, tds = 0, turb = 0;

    while (1) {
        bool has_new_data = false;

        // Lấy dữ liệu
        if(xSemaphoreTake(data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if(new_data) {
                temp = g_temp;
                ph   = g_ph;
                tds  = g_tds;
                turb = g_turb;
                new_data = false;  // ✅ đánh dấu đã lấy dữ liệu
                has_new_data = true;
            }
            xSemaphoreGive(data_mutex);  // ✅ luôn release mutex
        }

        // Vẽ TFT nếu có dữ liệu mới
        if(has_new_data) {
            if(xSemaphoreTake(tft_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                tft_display_data(temp, ph, tds, turb);
                xSemaphoreGive(tft_mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

