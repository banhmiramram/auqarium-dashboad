#include "driver/mcpwm.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "feeder.h"

#define SERVO_PIN  33
static const char *TAG = "FEEDER";

// Duty (%) tương ứng với servo quay 360°
#define SERVO_STOP_DUTY 7.5f   // Dừng
#define SERVO_ON_DUTY   9.5f   // Quay 1 chiều (đổi thành 5.5f nếu quay ngược)

// Hàm nội bộ thiết lập duty PWM
static void feeder_set_duty(float duty)
{
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    ESP_LOGI(TAG, "Servo duty = %.2f%%", duty);
}

// Giữ nguyên tên hàm cũ nhưng đổi logic phù hợp servo 360°
void feeder_set_angle(int angle)
{
    if (angle >= 90) {
        // Quay
        feeder_set_duty(SERVO_ON_DUTY);
        ESP_LOGI(TAG, "Servo ON (rotating)");
    } else {
        // Dừng
        feeder_set_duty(SERVO_STOP_DUTY);
        ESP_LOGI(TAG, "Servo OFF (stopped)");
    }
}

void feeder_init(void)
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_PIN);
    mcpwm_config_t pwm_config = {
        .frequency = 50, // SG90 yêu cầu 50Hz
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    feeder_set_duty(SERVO_STOP_DUTY); // Dừng ban đầu
    ESP_LOGI(TAG, "Feeder initialized (stopped)");
}
