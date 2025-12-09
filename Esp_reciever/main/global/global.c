#include "global.h"

// Sensor data
float g_temp = 0.0f;
float g_ph   = 0.0f;
float g_tds  = 0.0f;
float g_turb = 0.0f;
bool  new_data = false;

// Mutex
SemaphoreHandle_t data_mutex = NULL;
SemaphoreHandle_t tft_mutex = NULL;

// Warnings
warning_t warnings[MAX_WARNINGS];
int warning_count = 0;

// Network states
bool wifi_connected = false;
bool espnow_ready = false;

// Device state
bool motor_running = false;
bool feeder_active = false;
uint8_t feeder_level = 0;

// Queues
QueueHandle_t xEspNowQueue = NULL;
QueueHandle_t xWifiQueue   = NULL;
