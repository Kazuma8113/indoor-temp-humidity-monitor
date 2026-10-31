#include "bsp_motor.h"
#include "bsp_pin.h"
#include "tim.h"        /* htim2：调速 PWM 用的定时器 */

/* 方向脚与 STBY 直接走 HAL；调速走 TIM2_CH1 的比较寄存器。 */

void Bsp_Motor_Init(void) {
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    HAL_GPIO_WritePin(Motor.AIN1.Port, Motor.AIN1.Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Motor.AIN2.Port, Motor.AIN2.Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Motor.STBY.Port, Motor.STBY.Pin, GPIO_PIN_SET);   /* 使能驱动 */
}

void Bsp_Motor_Enable(uint8_t on) {
    HAL_GPIO_WritePin(Motor.STBY.Port, Motor.STBY.Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Bsp_Motor_SetPins(uint8_t ain1_level, uint8_t ain2_level) {
    HAL_GPIO_WritePin(Motor.AIN1.Port, Motor.AIN1.Pin,
                      ain1_level ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Motor.AIN2.Port, Motor.AIN2.Pin,
                      ain2_level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Bsp_Motor_SetDuty(uint16_t duty_percent) {
    uint32_t arr;
    uint32_t ccr;

    if (duty_percent > 100U) duty_percent = 100U;

    /* ARR 由 CubeMX 的 tim.c 决定（1kHz @ 当前时钟树），这里现取现算，
     * 以后改 ARR 不用同步修改本文件里的魔数。 */
    arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    ccr = ((uint32_t) duty_percent * arr) / 100U;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
}
