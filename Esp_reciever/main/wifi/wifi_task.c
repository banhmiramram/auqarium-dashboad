#include "global/global.h"
#include "wifi/wifi.h"
#include "esp_log.h"
#include "mqtt/my_mqtt_client.h"
#include "freertos/event_groups.h"

static const char *TAG = "WIFI_TASK";

void wifi_task(void *pv)
{
    sensor_data_t s;

    ESP_LOGI(TAG, "WiFi task started on core %d", xPortGetCoreID());

    // Đợi WiFi sẵn sàng
    vTaskDelay(pdMS_TO_TICKS(3000));

    // Start MQTT
    mqtt_app_start();

    while(1) {
        if (xQueueReceive(xWifiQueue, &s, portMAX_DELAY) == pdTRUE) {
            mqtt_publish_data(s.temp, s.ph, s.tds, s.turb);
        }
    }
}
