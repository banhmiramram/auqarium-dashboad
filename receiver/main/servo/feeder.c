#include "driver/mcpwm.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "feeder.h"

#define SERVO_PIN  25
static const char *TAG = "FEEDER";

static void feeder_set_angle_internal(int angle)
{
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;

    float duty = (0.5 + (angle / 180.0) * 2.0) / 20.0 * 100.0; // %
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);

    ESP_LOGI(TAG, "Servo set to %d° (duty = %.2f%%)", angle, duty);
}

void feeder_set_angle(int angle)
{
    feeder_set_angle_internal(angle);
}

void feeder_init(void)
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_PIN);
    mcpwm_config_t pwm_config = {
        .frequency = 50,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
}
