#ifndef BSP_MOTOR_H
#define BSP_MOTOR_H

#include <stdint.h>

/* ============================================================================
 * 板级封装：TB6612 电机驱动（方向脚 PE4/PE5、使能 PE6、调速 TIM2_CH1）
 *
 * 三件事：
 *   ① 方向脚 AIN1/AIN2 的电平组合
 *   ② STBY 使能
 *   ③ 调速 PWM（占空比 0~100 → CCR，ARR 现取现算，不写死魔数）
 * ========================================================================== */

void Bsp_Motor_Init(void);                  /* 启动 PWM、方向脚拉低、使能 STBY */
void Bsp_Motor_Enable(uint8_t on);          /* STBY：1 = 使能驱动 */
void Bsp_Motor_SetPins(uint8_t ain1_level, uint8_t ain2_level);
void Bsp_Motor_SetDuty(uint16_t duty_percent);   /* 0~100，超出截断 */

#endif /* BSP_MOTOR_H */
