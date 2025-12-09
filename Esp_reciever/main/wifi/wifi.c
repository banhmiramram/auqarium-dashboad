#include "wifi.h"
#include "global/global.h"      
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <string.h>

#define WIFI_SSID_STA "Thai Hiep"
#define WIFI_PASS_STA "thaiiihieppp"
#define WIFI_SSID_AP  "AquaBoard"
#define WIFI_PASS_AP  "12345678"

static const char *TAG = "WIFI";

/* ==== Event Handler ==== */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Connecting to WiFi (STA)...");
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (data_mutex && xSemaphoreTake(data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            wifi_connected = false;
            xSemaphoreGive(data_mutex);
        }
        ESP_LOGW(TAG, "Disconnected. Reconnecting...");
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (data_mutex && xSemaphoreTake(data_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            wifi_connected = true;
            xSemaphoreGive(data_mutex);
        }
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "✅ Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

/* ==== Khởi tạo Wi-Fi APSTA ==== */
void wifi_init_apsta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        ESP_ERROR_CHECK(err);

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_SSID_AP,
            .ssid_len = strlen(WIFI_SSID_AP),
            .channel = 1,
            .password = WIFI_PASS_AP,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(WIFI_PASS_AP) == 0)
        ap_config.ap.authmode = WIFI_AUTH_OPEN;

    wifi_config_t sta_config = {
        .sta = {
            .ssid = WIFI_SSID_STA,
            .password = WIFI_PASS_STA,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();

    ESP_LOGI(TAG, "✅ WiFi APSTA mode started — AP:%s, PASS:%s", WIFI_SSID_AP, WIFI_PASS_AP);
}
