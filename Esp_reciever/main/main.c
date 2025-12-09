#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "wifi/wifi.h"
#include "servo/feeder.h"
#include "motor/motor.h"
#include "tft/tft.h"
#include "espnow_recv/espnow_recv.h"
#include "global/global.h"
#include "manager_task/manager.h"
#include "mqtt/my_mqtt_client.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Booting...");

    // NVS
    ESP_ERROR_CHECK(nvs_flash_init());

    // Tạo mutex & queues
    data_mutex = xSemaphoreCreateMutex();
    tft_mutex = xSemaphoreCreateMutex();
    
    if (!data_mutex) {
        ESP_LOGE(TAG, "Failed to create data_mutex");
        return;
    }

    xEspNowQueue = xQueueCreate(16, sizeof(sensor_data_t)); // nhận nhanh từ espnow
    xWifiQueue   = xQueueCreate(8,  sizeof(sensor_data_t)); // gửi/ buffer cho wifi

    if (!xEspNowQueue || !xWifiQueue) {
        ESP_LOGE(TAG, "Failed to create queues");
        return;
    }

     // Khởi tạo phần cứng
    feeder_init();
    motor_init();
    tft_init();

    // Khởi tạo truyền thông
    wifi_init_apsta();      
    espnow_init();          

    // Tạo task: espnow_task (core 0), manager (no pin), display (core 1), wifi_task (core 1)
    xTaskCreatePinnedToCore(espnow_task, "espnow_task", 4096, NULL, 6, NULL, 0);
    xTaskCreatePinnedToCore(display_task, "display_task", 6144, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(wifi_task, "wifi_task", 8192, NULL, 4, NULL, 1);
    xTaskCreate(manager_task, "manager_task", 6144, NULL, 5, NULL);

    ESP_LOGI(TAG, "System started (queues+tasks created)");
}
