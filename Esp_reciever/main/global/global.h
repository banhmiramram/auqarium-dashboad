#pragma once
#ifndef GLOBAL_H
#define GLOBAL_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_WARNINGS 10

typedef struct {
    char type[16];
    char message[64];
    char time[16];
    int level;
} warning_t;

typedef struct {
    float temp;
    float ph;
    float tds;
    float turb;
} sensor_data_t;

// Sensor data (shared)
extern float g_temp;
extern float g_ph;
extern float g_tds;
extern float g_turb;
extern bool  new_data;

// Mutex to protect shared data
extern SemaphoreHandle_t data_mutex;
extern SemaphoreHandle_t tft_mutex;

// Warnings
extern warning_t warnings[MAX_WARNINGS];
extern int warning_count;

// Network states
extern bool wifi_connected;
extern bool espnow_ready;

// Device states
extern bool motor_running;
extern bool feeder_active;
extern uint8_t feeder_level;

// Queues for inter-task comms
extern QueueHandle_t xEspNowQueue; // from ESP-NOW callback -> manager
extern QueueHandle_t xWifiQueue;   // from manager -> wifi_task (send/upload)

#endif // GLOBAL_H
