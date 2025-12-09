#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "servo/feeder.h"
#include "motor/motor.h"
#include "espnow_recv/espnow_recv.h"
#include "global/global.h" 

static const char *TAG = "WEB_SERVER";

/* ================== HANDLERS ================== */

// ---- Gửi danh sách cảnh báo (warning) ----
esp_err_t warning_handler(httpd_req_t *req)
{
    char json[2048];
    strcpy(json, "[");

    // Khóa đọc dữ liệu cảnh báo
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    int count = warning_count;
    warning_t local_warnings[MAX_WARNINGS];
    memcpy(local_warnings, warnings, sizeof(warning_t) * count);
    xSemaphoreGive(data_mutex);

    // Tạo JSON phản hồi
    for (int i = 0; i < count; i++) {
        char entry[256];
        snprintf(entry, sizeof(entry),
                 "{\"type\":\"%s\",\"message\":\"%s\",\"level\":%d,\"time\":\"%s\"}%s",
                 local_warnings[i].type, local_warnings[i].message,
                 local_warnings[i].level, local_warnings[i].time,
                 (i == count - 1) ? "" : ",");
        if (strlen(json) + strlen(entry) < sizeof(json) - 1)
            strcat(json, entry);
    }

    strcat(json, "]");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, strlen(json));

    ESP_LOGI(TAG, "⚠️ Sent %d warnings successfully to web", count);
    return ESP_OK;
}

// ---- Xử lý lệnh điều khiển servo ----
esp_err_t servo_handler(httpd_req_t *req)
{
    char query[32], angle_str[8];
    int angle = 0;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
        if (httpd_query_key_value(query, "angle", angle_str, sizeof(angle_str)) == ESP_OK)
            angle = atoi(angle_str);

    // feeder_set_angle(angle);
    httpd_resp_sendstr(req, "OK");

    ESP_LOGI(TAG, "✅ Servo command sent successfully — angle = %d°", angle);
    return ESP_OK;
}

// ---- Xử lý bật/tắt relay ----
esp_err_t relay_handler(httpd_req_t *req)
{
    char query[16], cmd[8];

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
        if (httpd_query_key_value(query, "state", cmd, sizeof(cmd)) == ESP_OK)
        {
            if (strcmp(cmd, "on") == 0)
                motor_on();
            else if (strcmp(cmd, "off") == 0)
                motor_off();

            ESP_LOGI(TAG, "✅ Relay command received — state = %s", cmd);
        }

    httpd_resp_sendstr(req, "OK");
    ESP_LOGI(TAG, "⚡ Relay response sent successfully");
    return ESP_OK;
}

// ---- Trả dữ liệu cảm biến (JSON) ----
esp_err_t sensor_handler(httpd_req_t *req)
{
    char json[128];
    float temp, ph, tds, turb;

    // Khóa đọc dữ liệu cảm biến
    xSemaphoreTake(data_mutex, portMAX_DELAY);
    temp = g_temp;
    ph   = g_ph;
    tds  = g_tds;
    turb = g_turb;
    xSemaphoreGive(data_mutex);

    snprintf(json, sizeof(json),
             "{\"temp\":%.2f,\"ph\":%.2f,\"tds\":%.2f,\"turb\":%.2f}",
             temp, ph, tds, turb);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);

    ESP_LOGI(TAG, "📡 Sensor data sent: T=%.2f°C, pH=%.2f, TDS=%.2fppm, Turb=%.2fNTU",
             temp, ph, tds, turb);
    return ESP_OK;
}

/* ================== WEB SERVER INIT ================== */
void start_web_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &(httpd_uri_t){"/servo", HTTP_GET, servo_handler, NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){"/relay", HTTP_GET, relay_handler, NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){"/sensor", HTTP_GET, sensor_handler, NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){"/warning", HTTP_GET, warning_handler, NULL});

        ESP_LOGI(TAG, "🌐 Web server started successfully on port %d", config.server_port);
    }
    else
    {
        ESP_LOGE(TAG, "❌ Failed to start web server");
    }
}
