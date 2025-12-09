#include <stdio.h>
#include <string.h>
#include <math.h>

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

// ================== Global shared ==================
float g_temp = 0, g_ph = 0, g_tds = 0, g_turb = 0;

// ================== Peer MAC ==================
uint8_t peer_addr[6] = {0x10, 0x06, 0x1C, 0x86, 0xCE, 0x80};

// ================== ESP-NOW data struct ==================
typedef struct struct_message {
    float temp;
    float ph;
    float tds;
    float turb;
} struct_message;

struct_message sensorData;

// ================== Threshold config ==================
#define TEMP_DELTA     1.0f
#define PH_DELTA       0.2f
#define TDS_DELTA      4.0f
#define TURB_DELTA     2.0f

#define HEARTBEAT_MS     1800000   // 30 phút
#define DEBOUNCE_COUNT   2

// ================== Last sent values ==================
float last_temp = -999, last_ph = -999, last_tds = -999, last_turb = -999;

int temp_cnt = 0, ph_cnt = 0, tds_cnt = 0, turb_cnt = 0;

uint32_t last_send_time = 0;

// ================== Callback send ==================
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "ESP-NOW send: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ================== Init ESP-NOW ==================
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
    ESP_LOGI(TAG, "ESP-NOW initialized");
}

// ================== Check value delta ==================
bool value_changed(float now, float last, float delta)
{
    if (last < -900) return true;  // first time
    return fabs(now - last) >= delta;
}

// ================== Send task with threshold logic ==================
void espnow_send_task(void *pvParameter)
{
    while (1)
    {
        bool should_send = false;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

        sensorData.temp = g_temp;
        sensorData.ph   = g_ph;
        sensorData.tds  = g_tds;
        sensorData.turb = g_turb;

        // ===== Check delta with debounce =====
        if (value_changed(g_temp, last_temp, TEMP_DELTA)) {
            if (++temp_cnt >= DEBOUNCE_COUNT) { should_send = true; temp_cnt = 0; }
        } else temp_cnt = 0;

        if (value_changed(g_ph, last_ph, PH_DELTA)) {
            if (++ph_cnt >= DEBOUNCE_COUNT) { should_send = true; ph_cnt = 0; }
        } else ph_cnt = 0;

        if (value_changed(g_tds, last_tds, TDS_DELTA)) {
            if (++tds_cnt >= DEBOUNCE_COUNT) { should_send = true; tds_cnt = 0; }
        } else tds_cnt = 0;

        if (value_changed(g_turb, last_turb, TURB_DELTA)) {
            if (++turb_cnt >= DEBOUNCE_COUNT) { should_send = true; turb_cnt = 0; }
        } else turb_cnt = 0;

        // ===== Heartbeat 30 phút =====
        if (now - last_send_time > HEARTBEAT_MS) {
            should_send = true;
            ESP_LOGI(TAG, "HEARTBEAT: 30 minutes reached");
        }

        // ===== Send if required =====
        if (should_send)
        {
            esp_err_t result = esp_now_send(peer_addr, (uint8_t *)&sensorData, sizeof(sensorData));

            if (result == ESP_OK)
            {
                ESP_LOGI(TAG,
                    "Sent: Temp=%.2f PH=%.2f TDS=%.2f Turb=%.2f",
                    sensorData.temp, sensorData.ph, sensorData.tds, sensorData.turb);

                last_temp = g_temp;
                last_ph   = g_ph;
                last_tds  = g_tds;
                last_turb = g_turb;
                last_send_time = now;
            }
            else {
                ESP_LOGE(TAG, "ESP-NOW send error: %s", esp_err_to_name(result));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // check every 2 sec
    }
}

// ================== MAIN ==================
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    sensor_init();
    ds18b20_bb_init(DS_GPIO);

    espnow_init();

    xTaskCreate(espnow_send_task, "espnow_send_task", 4096, NULL, 4, NULL);

    while (1)
    {
        ds18b20_bb_read_temp(&g_temp);
        g_ph  = sensor_read_ph();
        g_tds = sensor_read_tds();
        g_turb = sensor_read_turb();

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
