#ifndef ESPNOW_RECV_H
#define ESPNOW_RECV_H

#include <stdbool.h>

typedef struct {
    char type[16];
    char message[64];
    int level;
    char time[32];
} warning_t;  

#define MAX_WARNINGS 20

extern warning_t warnings[MAX_WARNINGS];
extern int warning_count;

const warning_t *get_warnings(int *count);
void add_warning(const char *type, const char *msg, int level);
void espnow_init(void);
void check_sensor_levels(void);

#endif
