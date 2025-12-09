#include "espnow_recv.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"  // xPortGetCoreID()
#include "freertos/task.h"


static const char *TAG = "ESPNOW_TASK";

void espnow_task(void *pv)
{
    ESP_LOGI(TAG, "ESP-NOW task started on core %d", xPortGetCoreID());
    while (1) {
        // Task này không xử lý gì nặng — chủ yếu để giữ CPU core 0 bận
        // hoặc có thể dùng để kiểm tra trạng thái ESP-NOW định kỳ
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
