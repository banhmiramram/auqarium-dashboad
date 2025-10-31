#include <stdio.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "feeder.h"

// Biến toàn cục từ main.c
extern float g_temp, g_ph, g_tds, g_turb;

static const char *TAG = "WEB_SERVER";
extern const uint8_t servo_html_start[] asm("_binary_servo_html_start");
extern const uint8_t servo_html_end[]   asm("_binary_servo_html_end");

/* Trang HTML chính */
esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)servo_html_start, servo_html_end - servo_html_start);
    return ESP_OK;
}

/* API: điều khiển servo */
esp_err_t servo_handler(httpd_req_t *req)
{
    char query[32], angle_str[8];
    int angle = 0;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
        if (httpd_query_key_value(query, "angle", angle_str, sizeof(angle_str)) == ESP_OK)
            angle = atoi(angle_str);

    feeder_set_angle(angle);
    ESP_LOGI(TAG, "Servo set to %d°", angle);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

/* API: gửi dữ liệu cảm biến cho web (JSON) */
esp_err_t sensor_handler(httpd_req_t *req)
{
    char json[128];
    snprintf(json, sizeof(json),
             "{\"temp\":%.2f,\"ph\":%.2f,\"tds\":%.2f,\"turb\":%.2f}",
             g_temp, g_ph, g_tds, g_turb);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

/* Khởi tạo server */
void start_web_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &(httpd_uri_t){"/", HTTP_GET, root_get_handler, NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){"/servo", HTTP_GET, servo_handler, NULL});
        httpd_register_uri_handler(server, &(httpd_uri_t){"/sensor", HTTP_GET, sensor_handler, NULL});
        ESP_LOGI(TAG, "Web server started");
    }
}
