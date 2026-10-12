#ifndef GRADUATION_PROJECT_LED_H
#define GRADUATION_PROJECT_LED_H

#include "main.h"


extern GPIO_PinStruct LED_PC13;
extern uint8_t LED_State;

void LED_Flip(GPIO_PinStruct GPIO, uint32_t Time);
void LED_Off(GPIO_PinStruct GPIO);

/* ---- LED 状态的加锁访问 --------------------------------------------------
 * LED_State 被两个任务共享：LED_Task 读它决定"闪 or 灭"，
 * Bluetooth_Task 收到 LED_ON / LED_OFF 指令时写它。
 * 统一走下面两个函数，内部由 Mutex_LedState 保护，不要再直接访问 LED_State。
 * ⚠️ 绝对不能在持锁期间调用 LED_Flip —— 它内部 osDelayUntil 会阻塞 500ms。 */
void LED_State_Init(void);          /* 创建互斥锁，在 MX_FREERTOS_Init() 里调用 */
void LED_State_Set(uint8_t state);  /* 写 LED_State（加锁） */
uint8_t LED_State_Get(void);        /* 读 LED_State（加锁） */

#endif //GRADUATION_PROJECT_LED_H
