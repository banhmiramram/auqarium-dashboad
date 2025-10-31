#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "wifi/wifi.h"
#include "sensor/sensor.h"
#include "oled/oled.h"
#include "servo/feeder.h"
#include "ds18b20/ds18b20.h"
#include "web/web_server.h"

#define TAG "MAIN"
#define DS_GPIO GPIO_NUM_26

// Biến toàn cục chia sẻ với web_server
float g_temp = 0, g_ph = 0, g_tds = 0, g_turb = 0;

// MAC của ESP32 hiển thị
uint8_t peer_addr[6] = {0x10, 0x06, 0x1C, 0x86, 0xCE, 0x80};

// ================== Cấu trúc dữ liệu gửi qua ESP-NOW ==================
typedef struct struct_message {
    float temp;
    float ph;
    float tds;
    float turb;
} struct_message;

struct_message sensorData;

// ================== Callback khi gửi ==================
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "ESP-NOW send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ================== Khởi tạo ESP-NOW ==================
static void espnow_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(5, WIFI_SECOND_CHAN_NONE));


    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_cb));

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peer_addr, 6);
    peerInfo.channel = 5;
    peerInfo.encrypt = false;

    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));

    ESP_LOGI(TAG, "ESP-NOW initialized and peer added");
}

// ================== TASK GỬI DỮ LIỆU ==================
void espnow_send_task(void *pvParameter)
{
    while (1)
    {
        sensorData.temp = g_temp;
        sensorData.ph = g_ph;
        sensorData.tds = g_tds;
        sensorData.turb = g_turb;

        esp_err_t result = esp_now_send(peer_addr, (uint8_t *)&sensorData, sizeof(sensorData));
        if (result == ESP_OK)
        {
            ESP_LOGI(TAG, "ESP-NOW sent: T=%.2f PH=%.2f TDS=%.2f Turb=%.2f",
                     sensorData.temp, sensorData.ph, sensorData.tds, sensorData.turb);
        }
        else
        {
            ESP_LOGE(TAG, "ESP-NOW send error: %s", esp_err_to_name(result));
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}

// ================== MAIN ==================
void app_main(void)
{
    // --- 1. Khởi tạo cơ bản ---
    ESP_ERROR_CHECK(nvs_flash_init());
    // wifi_init_sta(); // nếu bạn cần Wi-Fi riêng để kết nối web (có thể bỏ nếu chỉ ESP-NOW)
    sensor_init();
    // feeder_init();
    ds18b20_bb_init(DS_GPIO);
    // oled_init();
    // start_web_server();

    // --- 2. Khởi tạo ESP-NOW ---
    espnow_init();

    // --- 3. Tạo task gửi dữ liệu song song ---
    xTaskCreate(espnow_send_task, "espnow_send_task", 4096, NULL, 4, NULL);

    // --- 4. Vòng lặp đọc cảm biến ---
    while (1)
    {
        ds18b20_bb_read_temp(&g_temp);
        g_ph = sensor_read_ph();
        g_tds = sensor_read_tds();
        g_turb = sensor_read_turb();

        // Có thể hiển thị nội bộ (OLED)
        // oled_display_values(g_temp, g_ph, g_tds, g_turb);

        ESP_LOGI(TAG, "Sensor: Temp=%.2f, PH=%.2f, TDS=%.2f, Turb=%.2f",
                 g_temp, g_ph, g_tds, g_turb);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
