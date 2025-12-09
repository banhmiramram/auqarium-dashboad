#include "espnow_recv.h"
#include "global/global.h"         
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "string.h"
#include "tft.h"
#include <time.h>

#define TAG "ESP_NOW"

/* ==== Cấu trúc dữ liệu nhận từ node ESP-NOW ==== */
typedef struct {
    float temp;
    float ph;
    float tds;
    float turb;
} struct_message;

static struct_message incomingData;

const warning_t *get_warnings(int *count)
{
    *count = warning_count;
    return warnings;
}

void add_warning(const char *type, const char *msg, int level)
{
    if (warning_count >= MAX_WARNINGS) {
        for (int i = 1; i < MAX_WARNINGS; i++)
            warnings[i - 1] = warnings[i];
        warning_count = MAX_WARNINGS - 1;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    snprintf(warnings[warning_count].type, sizeof(warnings[warning_count].type), "%s", type);
    snprintf(warnings[warning_count].message, sizeof(warnings[warning_count].message), "%s", msg);
    snprintf(warnings[warning_count].time, sizeof(warnings[warning_count].time),
             "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    warnings[warning_count].level = level;

    warning_count++;
}

/* (Tuỳ chọn) Kiểm tra ngưỡng cảm biến */
void check_sensor_levels(void)
{
    // Nhiệt độ
    if (g_temp > 38.0)
        add_warning("Temp", "Nhiệt độ vượt 38°C", 2);
    else if (g_temp > 32.0)
        add_warning("Temp", "Nhiệt độ cao hơn 32°C", 1);

    // pH
    if (g_ph < 6.0)
        add_warning("pH", "pH quá thấp (<6.0)", 2);
    else if (g_ph < 6.5)
        add_warning("pH", "pH thấp nhẹ (<6.5)", 1);
    else if (g_ph > 8.5)
        add_warning("pH", "pH cao (>8.5)", 1);

    // TDS
    if (g_tds > 1500)
        add_warning("TDS", "TDS rất cao (>1500ppm)", 2);
    else if (g_tds > 1000)
        add_warning("TDS", "TDS cao (>1000ppm)", 1);

    // Độ đục
    if (g_turb > 8)
        add_warning("Turb", "Nước rất đục (>8NTU)", 2);
    else if (g_turb > 5)
        add_warning("Turb", "Độ đục cao (>5NTU)", 1);
}

/* ==========================================================
   CALLBACK NHẬN DỮ LIỆU TỪ ESP-NOW
   ========================================================== */
static void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    if (len == sizeof(struct_message)) {
        memcpy(&incomingData, data, sizeof(incomingData));

        sensor_data_t s;
        s.temp = incomingData.temp;
        s.ph   = incomingData.ph;
        s.tds  = incomingData.tds;
        s.turb = incomingData.turb;

        BaseType_t xHigher = pdFALSE;
        if (xEspNowQueue) {
            // callback context: use non-blocking send
            if (xQueueSend(xEspNowQueue, &s, 0) != pdTRUE) {
                // queue đầy -> drop hoặc xử lý fallback
                ESP_LOGW(TAG, "xEspNowQueue full, dropping data");
            } else {
                // optional log
                ESP_LOGI(TAG, "ESP-NOW queued: T=%.2f pH=%.2f", s.temp, s.ph);
            }
        }
    } else {
        ESP_LOGW(TAG, "Received unknown data length: %d", len);
    }
}

/* ==========================================================
   KHỞI TẠO ESP-NOW
   ========================================================== */
void espnow_init(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
    espnow_ready = true;
    ESP_LOGI(TAG, "ESP-NOW initialized");
}