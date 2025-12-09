#include "ds18b20.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"


// Delay us
static void ds_delay_us(uint32_t us) {
    esp_rom_delay_us(us);
}

static const char *TAG = "DS18B20_BB";

static gpio_num_t ds_gpio = GPIO_NUM_NC;

// Hàm reset bus 1-Wire, trả về true nếu có presence
static bool ds18b20_bb_reset(void) {
    // kéo bus xuống 0 trong > 480µs
    gpio_set_direction(ds_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(ds_gpio, 0);
    ds_delay_us(480);

    // thả, chuyển thành input
    gpio_set_direction(ds_gpio, GPIO_MODE_INPUT);
    ds_delay_us(70);

    // đọc presence (slave kéo thấp)
    int presence = gpio_get_level(ds_gpio);
    ds_delay_us(410);

    return (presence == 0);
}

// Ghi 1 bit trên bus
static void ds18b20_bb_write_bit(int bit) {
    gpio_set_direction(ds_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(ds_gpio, 0);
    if (bit) {
        // viết bit “1”: kéo thấp ngắn, sau đó thả
        ds_delay_us(6);
        gpio_set_level(ds_gpio, 1);
        ds_delay_us(64);
    } else {
        // viết bit “0”: giữ thấp lâu hơn
        ds_delay_us(60);
        gpio_set_level(ds_gpio, 1);
        ds_delay_us(10);
    }
}

// Đọc 1 bit từ bus
static int ds18b20_bb_read_bit(void) {
    int bit;
    gpio_set_direction(ds_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(ds_gpio, 0);
    ds_delay_us(6);

    gpio_set_direction(ds_gpio, GPIO_MODE_INPUT);
    ds_delay_us(9);

    bit = gpio_get_level(ds_gpio);
    ds_delay_us(55);
    return bit;
}

// Ghi 1 byte (LSB trước)
static void ds18b20_bb_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        ds18b20_bb_write_bit(data & 0x01);
        data >>= 1;
    }
}

// Đọc 1 byte
static uint8_t ds18b20_bb_read_byte(void) {
    uint8_t v = 0;
    for (int i = 0; i < 8; i++) {
        if (ds18b20_bb_read_bit()) {
            v |= (1 << i);
        }
    }
    return v;
}

// Tính CRC8 cho scratchpad (0..7 bytes)
static uint8_t ds_crc8(const uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

// Khởi tạo
esp_err_t ds18b20_bb_init(gpio_num_t gpio_num) {
    ds_gpio = gpio_num;
    // Kéo lên (pull-up) — cần điện trở kéo lên ~4.7k ngoài
    gpio_set_direction(ds_gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ds_gpio, GPIO_PULLUP_ONLY);
    // reset kiểm tra presence
    if (!ds18b20_bb_reset()) {
        ESP_LOGE(TAG, "No presence pulse");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "DS18B20 presence detected on GPIO %d", ds_gpio);
    return ESP_OK;
}

// Đọc nhiệt độ
esp_err_t ds18b20_bb_read_temp(float *temperature) {
    if (temperature == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!ds18b20_bb_reset()) {
        ESP_LOGE(TAG, "Reset failed");
        return ESP_FAIL;
    }
    // ROM command: Skip ROM (CCh) nếu chỉ 1 thiết bị
    ds18b20_bb_write_byte(0xCC);
    // Chức năng: Convert T = 0x44
    ds18b20_bb_write_byte(0x44);

    // Chờ chuyển đổi (tùy độ phân giải). Với 12-bit: đến 750 ms
    vTaskDelay(pdMS_TO_TICKS(750));

    // Reset lại bus
    if (!ds18b20_bb_reset()) {
        ESP_LOGE(TAG, "Reset after convert failed");
        return ESP_FAIL;
    }
    ds18b20_bb_write_byte(0xCC);
    ds18b20_bb_write_byte(0xBE);  // Read Scratchpad

    uint8_t sp[9];
    for (int i = 0; i < 9; i++) {
        sp[i] = ds18b20_bb_read_byte();
    }

    // CRC kiểm tra
    uint8_t crc = ds_crc8(sp, 8);
    if (crc != sp[8]) {
        ESP_LOGE(TAG, "CRC error: calc=0x%02X, rx=0x%02X", crc, sp[8]);
        return ESP_FAIL;
    }

    // ghép MSB, LSB
    int16_t raw = ( (int16_t)sp[1] << 8 ) | sp[0];
    // giá trị raw theo datasheet, 1/16 độ C
    float temp_c = raw / 16.0;

    *temperature = temp_c;
    ESP_LOGI(TAG, "Temperature = %.2f°C", temp_c);
    return ESP_OK;
}
