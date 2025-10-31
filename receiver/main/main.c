#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "wifi/wifi.h"
#include "web/web_server.h"
#include "servo/feeder.h"
#include "motor/motor.h"
#include "tft/tft.h"
#include "espnow_recv/espnow_recv.h"
#include <stdbool.h>

#define TAG "MAIN"

extern float g_temp, g_ph, g_tds, g_turb;
extern bool new_data;

void display_task(void *pv)
{
    while (1) {
        if (new_data) {
            new_data = false; // reset flag

            ESP_LOGI("DISPLAY", "Update TFT: T=%.2f PH=%.2f TDS=%.2f Turb=%.2f",
                     g_temp, g_ph, g_tds, g_turb);

            tft_display_data(g_temp, g_ph, g_tds, g_turb);
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // refresh mỗi 5s
    }
}

void warning_task(void *pv)
{
    while (1) {
        check_sensor_levels();   // kiểm tra và thêm cảnh báo nếu cần
        vTaskDelay(pdMS_TO_TICKS(10000));  // kiểm tra mỗi 2 giây
    }
}


void app_main(void)
{
    // 1. NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    // 2. Khởi tạo WiFi STA để có Internet / local web
    wifi_init_apsta();
    // 4. Khởi tạo ESP-NOW (dùng chung WiFi STA)
    espnow_init();
    // 5. Khởi tạo màn hình TFT
    tft_init();
     // Khởi tạo feeder servo
    feeder_init();
    // Khởi tạo motor relay
    motor_init();
    // 6. Khởi tạo web server local
    start_web_server();

    xTaskCreate(display_task, "display_task", 4096, NULL, 2, NULL);

    // xTaskCreate(warning_task, "warning_task", 4096, NULL, 2, NULL);

    ESP_LOGI("MAIN", "System ready!");
}

