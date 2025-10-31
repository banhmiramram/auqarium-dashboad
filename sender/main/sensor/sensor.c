#include "sensor.h"
#include "driver/adc.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "SENSOR";

#define VREF 3.3
#define ADC_RES 4095.0

#define TURB_V_CLEAN 2.50
#define TURB_V_DIRTY 0.40
#define TURB_MAX_NTU 1000.0

esp_err_t sensor_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);

    adc1_config_channel_atten(PH_PIN,   ADC_ATTEN_DB_11);
    adc1_config_channel_atten(TDS_PIN,  ADC_ATTEN_DB_11);
    adc1_config_channel_atten(TURB_PIN, ADC_ATTEN_DB_11);

    ESP_LOGI(TAG, "ADC initialized for pH, TDS, Turbidity sensors.");
    return ESP_OK;
}


static float to_voltage(int raw)
{
    return (raw * VREF) / ADC_RES;
}

// -------------------- PH --------------------
float sensor_read_ph(void)
{
    int raw = adc1_get_raw(PH_PIN);
    float v = to_voltage(raw);
    float ph = 7.0 + ((2.5 - v) * 3.5);
    ESP_LOGI(TAG, "pH Raw: %d | Volt: %.3f | pH: %.2f", raw, v, ph);
    return ph;
}

// -------------------- TDS --------------------
float sensor_read_tds(void)
{
    int raw = adc1_get_raw(TDS_PIN);
    float v = to_voltage(raw);

    // DFROBOT style (ước lượng, t°=25°C)
    float compCoeff = 1.0 + 0.02 * (25.0 - 25.0);
    float compV = v / compCoeff;
    float tds = (133.42 * compV * compV * compV
                - 255.86 * compV * compV
                + 857.39 * compV) * 0.5;
    if (tds < 0) tds = 0;
    ESP_LOGI(TAG, "TDS Raw: %d | Volt: %.3f | ppm: %.1f", raw, v, tds);
    return tds;
}

// -------------------- TURBIDITY --------------------
float sensor_read_turb(void)
{
    int raw = adc1_get_raw(TURB_PIN);
    float v = to_voltage(raw);

    float ntu;
    if (v <= 0.01)
        ntu = TURB_MAX_NTU;
    else if (v >= TURB_V_CLEAN)
        ntu = 0.0;
    else if (v <= TURB_V_DIRTY)
        ntu = TURB_MAX_NTU;
    else
        ntu = (TURB_V_CLEAN - v) / (TURB_V_CLEAN - TURB_V_DIRTY) * TURB_MAX_NTU;

    if (ntu < 0) ntu = 0;
    if (ntu > TURB_MAX_NTU) ntu = TURB_MAX_NTU;

    ESP_LOGI(TAG, "TURB Raw: %d | Volt: %.3f | NTU: %.1f", raw, v, ntu);
    return ntu;
}
