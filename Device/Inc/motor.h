#ifndef DEVICE_MOTOR_H
#define DEVICE_MOTOR_H

#include "main.h"

// 电机方向枚举
typedef enum {
    MOTOR_STOP = 0,
    MOTOR_FORWARD,
    MOTOR_BACKWARD
} MotorDir;


void Motor_Init(void);
void Motor_SetDir(MotorDir dir);
void Motor_SetSpeed(uint16_t duty);

#endif /* DEVICE_MOTOR_H */
