#include "TB6612.h"

#include "bsp_motor.h"   /* 方向脚 / STBY / PWM，全部由 Bsp 直调 HAL */

/* 初始化电机（启动 PWM + 方向脚拉低 + 使能驱动） */
void Motor_Init(void) {
    Bsp_Motor_Init();               /* PWM 启动 + 方向脚拉低 + STBY 使能 */
    Motor_SetDir(MOTOR_FORWARD);    /* 设默认方向 */
}

/* 设置电机方向 */
void Motor_SetDir(MotorDir dir) {
    switch (dir) {
        case MOTOR_FORWARD:
            Bsp_Motor_SetPins(1U, 0U);
            break;

        case MOTOR_BACKWARD:
            Bsp_Motor_SetPins(0U, 1U);
            break;

        case MOTOR_STOP:
        default:
            Bsp_Motor_SetPins(0U, 0U);
            break;
    }
}

/* 设置电机速度 (占空比 0~100%) */
void Motor_SetSpeed(uint16_t duty) {
    Bsp_Motor_SetDuty(duty);        /* 截断与 ARR 换算都在 Bsp 里 */
}
