#include "espnow_recv.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "string.h"
#include "tft.h"
#include <time.h>

#define TAG "ESP_NOW"

typedef struct {
    float temp;
    float ph;
    float tds;
    float turb;
} struct_message;

static struct_message incomingData;

bool new_data = false;

float g_temp = 0, g_ph = 0, g_tds = 0, g_turb = 0;

warning_t warnings[MAX_WARNINGS];
int warning_count = 0;

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

static void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    if (len == sizeof(struct_message)) {
        memcpy(&incomingData, data, sizeof(incomingData));
        g_temp = incomingData.temp;
        g_ph   = incomingData.ph;
        g_tds  = incomingData.tds;
        g_turb = incomingData.turb;
        new_data = true; 
        
    } else {
        ESP_LOGW(TAG, "Received unknown data length: %d", len);
    }
}

void espnow_init(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
    ESP_LOGI(TAG, "ESP-NOW Receiver initialized");
}
