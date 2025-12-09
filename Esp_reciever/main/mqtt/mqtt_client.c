#include "esp_log.h"
#include "mqtt_client.h"
#include "my_mqtt_client.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "motor.h"
#include "feeder.h"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;

// ====================== Xử lý gói lệnh điều khiển ======================
static void handle_control_message(const char *topic, const char *payload)
{
    ESP_LOGI(TAG, "Received control message on topic: %s", topic);
    ESP_LOGI(TAG, "Payload: %s", payload);

    // === Điều khiển relay ===
    if (strstr(topic, "/relay")) {
        cJSON *root = cJSON_Parse(payload);
        if (root) {
            cJSON *state = cJSON_GetObjectItem(root, "state");
            if (state && cJSON_IsString(state)) {
                if (strcmp(state->valuestring, "on") == 0) {
                    motor_on();
                } else {
                    motor_off();
                }
            }
            cJSON_Delete(root);
        }
    }

    // === Điều khiển servo ===
    else if (strstr(topic, "/servo")) {
        cJSON *root = cJSON_Parse(payload);
        if (root) {
            // Có thể nhận cả dạng { "angle": 180 } hoặc { "state": "on" }
            cJSON *angle = cJSON_GetObjectItem(root, "angle");
            cJSON *state = cJSON_GetObjectItem(root, "state");

            if (angle && cJSON_IsNumber(angle)) {
                feeder_set_angle(angle->valueint);
            }
            else if (state && cJSON_IsString(state)) {
                if (strcmp(state->valuestring, "on") == 0)
                    feeder_set_angle(180); // quay
                else
                    feeder_set_angle(0);   // dừng
            }

            cJSON_Delete(root);
        }
    }
}

// ====================== Callback MQTT ======================
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;

    switch ((esp_mqtt_event_id_t) event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");

            // Subscribe các topic điều khiển
            esp_mqtt_client_subscribe(client, "/aquarium/control/relay", 1);
            esp_mqtt_client_subscribe(client, "/aquarium/control/servo", 1);
            ESP_LOGI(TAG, "Subscribed to control topics");
            break;

        case MQTT_EVENT_DATA: {
            // Nhận dữ liệu từ broker
            char topic[event->topic_len + 1];
            char payload[event->data_len + 1];

            memcpy(topic, event->topic, event->topic_len);
            topic[event->topic_len] = '\0';

            memcpy(payload, event->data, event->data_len);
            payload[event->data_len] = '\0';

            handle_control_message(topic, payload);
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected, reconnecting...");
            esp_mqtt_client_reconnect(client);
            break;

        default:
            break;
    }
}

// ====================== Khởi tạo MQTT ======================
void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org",
        .network.disable_auto_reconnect = false,
        .session.keepalive = 60,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_err_t ret = esp_mqtt_client_start(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %d", ret);
    } else {
        ESP_LOGI(TAG, "MQTT client started");
    }
}

// ====================== Gửi dữ liệu cảm biến ======================
void mqtt_publish_data(float temp, float ph, float tds, float turb)
{
    if (client == NULL)
        return;

    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
        return;

    cJSON_AddNumberToObject(root, "temp", temp);
    cJSON_AddNumberToObject(root, "ph", ph);
    cJSON_AddNumberToObject(root, "tds", tds);
    cJSON_AddNumberToObject(root, "turb", turb);

    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        esp_mqtt_client_publish(client, "/aquarium/data", json_str, 0, 1, 0);
        free(json_str);
    }

    cJSON_Delete(root);
    vTaskDelay(pdMS_TO_TICKS(2000));
}
