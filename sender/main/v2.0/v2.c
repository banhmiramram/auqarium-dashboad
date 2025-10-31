#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi.h"
#include "sensor.h"
#include "oled.h"
#include "feeder.h"
#include "ds18b20.h"
#include "web_server.h"

#define TAG "MAIN"
#define DS_GPIO GPIO_NUM_26

// Biến toàn cục chia sẻ với web_server
float g_temp = 0, g_ph = 0, g_tds = 0, g_turb = 0;

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    sensor_init();
    feeder_init();
    ds18b20_bb_init(DS_GPIO);
    oled_init();

    start_web_server(); // ✅ khởi động web server nội bộ

    while (1)
    {
        // Đọc cảm biến
        ds18b20_bb_read_temp(&g_temp);
        g_ph = sensor_read_ph();
        g_tds = sensor_read_tds();
        g_turb = sensor_read_turb();

        // Hiển thị lên OLED
        // oled_display_values(g_temp, g_ph, g_tds, g_turb);

        // Cập nhật mỗi 2s
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
