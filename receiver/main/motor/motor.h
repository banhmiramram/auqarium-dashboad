#ifndef MOTOR_H
#define MOTOR_H

#include "driver/gpio.h"

// Chân điều khiển relay
#define RELAY_GPIO  GPIO_NUM_26

void motor_init(void);
void motor_on(void);
void motor_off(void);

#endif
