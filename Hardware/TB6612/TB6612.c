#include "TB6612.h"

#include "tim.h"

// TB6612 接线如下：
// AIN1 -> PA1
// AIN2 -> PA2
// PWMA -> TIM2_CH1 (PA0)
// STBY -> PA3

GPIO_PinStruct AIN1 = {GPIOE, GPIO_PIN_4};
GPIO_PinStruct AIN2 = {GPIOE, GPIO_PIN_5};
GPIO_PinStruct STBY = {GPIOE, GPIO_PIN_6};
GPIO_PinStruct PWMA = {GPIOA, GPIO_PIN_15};

// 初始化电机（启动 PWM）
void Motor_Init(void) {
    // 启动 TIM2 CH1 PWM
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    // 默认方向引脚拉低
    HAL_GPIO_WritePin(AIN1.Port, AIN1.Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIN2.Port, AIN2.Pin, GPIO_PIN_RESET);

    // 使能 TB6612
    HAL_GPIO_WritePin(STBY.Port, STBY.Pin, GPIO_PIN_SET);

    // 设置默认方向
    Motor_SetDir(MOTOR_FORWARD);
}

// 设置电机方向
void Motor_SetDir(MotorDir dir) {
    switch (dir) {
        case MOTOR_FORWARD:
            HAL_GPIO_WritePin(AIN1.Port, AIN1.Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(AIN2.Port, AIN2.Pin, GPIO_PIN_RESET);
            break;

        case MOTOR_BACKWARD:
            HAL_GPIO_WritePin(AIN1.Port, AIN1.Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(AIN2.Port, AIN2.Pin, GPIO_PIN_SET);
            break;

        case MOTOR_STOP:
        default:
            HAL_GPIO_WritePin(AIN1.Port, AIN1.Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(AIN2.Port, AIN2.Pin, GPIO_PIN_RESET);
            break;
    }
}

// 设置电机速度 (占空比 0~100%)
void Motor_SetSpeed(uint16_t duty) {
    if (duty > 100) duty = 100; // 限制最大值

    // 计算 CCR 值
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    uint32_t ccr = (duty * arr) / 100;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
}
