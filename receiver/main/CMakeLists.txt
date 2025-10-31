idf_component_register(
    SRCS "main.c"
         "tft/tft.c"
         "espnow_recv/espnow_recv.c"
         "web/web_server.c"
         "servo/feeder.c"
         "wifi/wifi.c"
         "motor/motor.c"
    INCLUDE_DIRS "." "web" "servo" "wifi" "motor" "tft" "espnow_recv"

    REQUIRES esp_lcd driver esp_wifi esp_event esp_netif nvs_flash esp_http_server freertos

    EMBED_FILES
        "servo.html"
)
