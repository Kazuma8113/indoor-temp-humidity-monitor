#ifndef GRADUATION_PROJECT_LED_H
#define GRADUATION_PROJECT_LED_H

#include "main.h"


extern GPIO_PinStruct LED_PC13;
extern uint8_t LED_State;

/* LED_State 的取值 —— LED_Task 按这个值决定怎么亮，其它模块（比如 Init_Task
 * 发现 DHT11 起不来、蓝牙收到 LED_ON/OFF 指令）通过 LED_State_Set() 改它。 */
#define LED_STATE_OFF         0U   /* 常灭 */
#define LED_STATE_ON          1U   /* 0.5s 一翻转（正常运行心跳） */
#define LED_STATE_DHT11_FAIL  2U   /* DHT11 初始化失败：0.2s 快闪 */

void LED_Flip(GPIO_PinStruct GPIO, uint32_t Time);
void LED_Off(GPIO_PinStruct GPIO);

void LED_State_Init(void);          /* 创建互斥锁，在 MX_FREERTOS_Init() 里调用 */
void LED_State_Set(uint8_t state);  /* 写 LED_State（加锁） */
uint8_t LED_State_Get(void);        /* 读 LED_State（加锁） */

#endif //GRADUATION_PROJECT_LED_H
