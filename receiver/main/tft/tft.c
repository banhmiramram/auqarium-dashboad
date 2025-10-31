#include "tft.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "string.h"
#include "font8x8_basic.h"

#define TAG "TFT"

// ==== Chân TFT ====
#define PIN_NUM_MOSI 23
#define PIN_NUM_SCK  18
#define PIN_NUM_CS   5
#define PIN_NUM_DC   2
#define PIN_NUM_RST  4
#define PIN_NUM_BK   21
#define LCD_HOST SPI2_HOST

static esp_lcd_panel_handle_t panel_handle = NULL;

// ==== Vẽ 1 pixel đơn ====
static inline void tft_draw_pixel(int x, int y, uint16_t color)
{
    esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + 1, y + 1, &color);
}

// ==== Vẽ hình chữ nhật rỗng ====
void tft_draw_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int i = x; i < x + w; i++) {
        tft_draw_pixel(i, y, color);
        tft_draw_pixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        tft_draw_pixel(x, j, color);
        tft_draw_pixel(x + w - 1, j, color);
    }
}

// ==== Vẽ vòng tròn rỗng (Bresenham) ====
void tft_draw_circle(int x0, int y0, int r, uint16_t color)
{
    int x = r;
    int y = 0;
    int err = 0;

    while (x >= y) {
        tft_draw_pixel(x0 + x, y0 + y, color);
        tft_draw_pixel(x0 + y, y0 + x, color);
        tft_draw_pixel(x0 - y, y0 + x, color);
        tft_draw_pixel(x0 - x, y0 + y, color);
        tft_draw_pixel(x0 - x, y0 - y, color);
        tft_draw_pixel(x0 - y, y0 - x, color);
        tft_draw_pixel(x0 + y, y0 - x, color);
        tft_draw_pixel(x0 + x, y0 - y, color);

        y++;
        if (err <= 0)
            err += 2 * y + 1;
        else {
            x--;
            err -= 2 * x + 1;
        }
    }
}

// ==== Vẽ vòng tròn tô kín ====
void tft_draw_filled_circle(int x0, int y0, int r, uint16_t color)
{
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r)
                tft_draw_pixel(x0 + x, y0 + y, color);
        }
    }
}

// ==== Vẽ ký tự có scale ====
void tft_draw_char_scaled(int x, int y, char c, uint16_t color, int scale)
{
    if (c < 0x20 || c >= 128) return;
    const uint8_t *bitmap = (const uint8_t *)font8x8_basic[(unsigned char)c];

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (bitmap[row] & (1 << col)) {
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        uint16_t pixel = color;
                        esp_lcd_panel_draw_bitmap(panel_handle,
                            x + col * scale + dx, y + row * scale + dy,
                            x + col * scale + dx + 1, y + row * scale + dy + 1,
                            &pixel);
                    }
                }
            }
        }
    }
}

// ==== Vẽ chuỗi có scale ====
void tft_draw_string_scaled(int x, int y, const char *str, uint16_t color, float scale)
{
    while (*str) {
        tft_draw_char_scaled(x, y, *str, color, scale);
        x += 8 * scale;
        str++;
    }
}

// ==== Xóa toàn màn hình ====
void tft_clear_screen(uint16_t color)
{
    uint16_t line[240];
    for (int i = 0; i < 240; i++) line[i] = color;
    for (int y = 0; y < 320; y++)
        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, 240, y + 1, line);
}

// ==== Vẽ tam giác rỗng (bổ sung) ====
void tft_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t color)
{
    // Thuật toán Bresenham cơ bản cho đoạn thẳng
    int dx, dy, sx, sy, err, e2;
    int x[3] = {x1, x2, x3};
    int y[3] = {y1, y2, y3};

    for (int i = 0; i < 3; i++) {
        int x0 = x[i], y0 = y[i];
        int x1_ = x[(i + 1) % 3], y1_ = y[(i + 1) % 3];
        dx = abs(x1_ - x0);
        sx = x0 < x1_ ? 1 : -1;
        dy = -abs(y1_ - y0);
        sy = y0 < y1_ ? 1 : -1;
        err = dx + dy;

        while (1) {
            tft_draw_pixel(x0, y0, color);
            if (x0 == x1_ && y0 == y1_) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }
}

// ==== HIỂN THỊ DỮ LIỆU CẢI TIẾN ====
void tft_display_data(float temp, float ph, float tds, float turb)
{
    // --- Nền đen ---
    uint16_t bg_color = 0x0000;
    tft_clear_screen(bg_color);

    // --- Màu sắc ---
    uint16_t white       = 0xFFFF; // Khung trắng
    uint16_t blue_light  = 0x5D9B; // Xanh dương nhạt
    uint16_t blue_icon   = 0x07FF; // Xanh ngọc sáng
    uint16_t LIGHT_BLUE  = 0x5DDF; // Tiêu đề
    uint16_t blue_title  = 0x001F; // Xanh đậm

    // --- Kích thước và bố cục ---
    int screen_w = 240;
    int scale_title = 2;
    int scale_data  = 2;
    int frame_x = 15, frame_y = 50, frame_w = 210, frame_h = 240;
    int line_h = 45;
    int text_x = frame_x + 35;
    int start_y = frame_y + 25;

    // --- Tiêu đề căn giữa ---
    const char *title = "AQUARIUM BOARD";
    int title_w = strlen(title) * 8 * scale_title;
    int title_x = (screen_w - title_w) / 2;
    tft_draw_string_scaled(title_x, 15, title, LIGHT_BLUE, scale_title);

    // --- Khung trắng ---
    tft_draw_rect(frame_x, frame_y, frame_w, frame_h, white);

    // --- Nhiệt độ ---
    char buf[32];
    sprintf(buf, "TEMP %.1fC", temp);
    tft_draw_circle(frame_x + 15, start_y + 6, 4, blue_icon);
    tft_draw_string_scaled(text_x, start_y, buf, blue_light, scale_data);

    // --- pH ---
    sprintf(buf, "PH   %.2f", ph);
    tft_draw_filled_circle(frame_x + 15, start_y + line_h + 6, 3, blue_icon);
    tft_draw_string_scaled(text_x, start_y + line_h, buf, blue_light, scale_data);

    // --- TDS ---
    sprintf(buf, "TDS  %.0f ppm", tds);
    tft_draw_triangle(frame_x + 12, start_y + 2 * line_h + 10,
                      frame_x + 15, start_y + 2 * line_h + 3,
                      frame_x + 18, start_y + 2 * line_h + 10, blue_icon);
    tft_draw_string_scaled(text_x, start_y + 2 * line_h, buf, blue_light, scale_data);

    // --- TURB ---
    sprintf(buf, "TURB %.1f", turb);
    tft_draw_circle(frame_x + 15, start_y + 3 * line_h + 6, 3, blue_icon);
    tft_draw_string_scaled(text_x, start_y + 3 * line_h, buf, blue_light, scale_data);

    // --- Footer căn giữa ---
    const char *footer = "live update";
    int footer_w = strlen(footer) * 8;
    int footer_x = (screen_w - footer_w) / 2;
    tft_draw_string_scaled(footer_x, 300, footer, LIGHT_BLUE, 1);
}

void tft_init(void)
{
    ESP_LOGI(TAG, "Init SPI bus...");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240 * 2 + 10,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 10 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    if (PIN_NUM_BK != -1) {
        gpio_set_direction(PIN_NUM_BK, GPIO_MODE_OUTPUT);
        gpio_set_level(PIN_NUM_BK, 1);
    }

    ESP_LOGI(TAG, "TFT ST7789 initialized");
}

