#include "motor.h"
#include "esp_log.h"

static const char *TAG = "MOTOR";

void motor_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Tắt relay ban đầu (vì relay kích mức HIGH)
    gpio_set_level(RELAY_GPIO, 0);
    ESP_LOGI(TAG, "Relay initialized at GPIO %d", RELAY_GPIO);
}

void motor_on(void)
{
    gpio_set_level(RELAY_GPIO, 1);
    ESP_LOGI(TAG, "Relay ON");
}

void motor_off(void)
{
    gpio_set_level(RELAY_GPIO, 0);
    ESP_LOGI(TAG, "Relay OFF");
}
