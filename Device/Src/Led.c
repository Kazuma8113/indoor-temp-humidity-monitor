#include "Led.h"
#include "bsp_led.h"      /* Bsp_Led_On / Off / Toggle / Init（HAL 直调，引脚内置） */
#include "cmsis_os.h"     /* osDelay */

/* 只做电气动作 + 节奏延时；业务状态机（心跳/快闪/常亮）在 App/Src/app_led.c */

void LED_Init(void) {
    Bsp_Led_Init();     /* 初始化为熄灭 */
}

void LED_Flip(uint32_t Time) {
    uint32_t TimeStart;

    Bsp_Led_Toggle();
    TimeStart = osKernelGetTickCount();
    osDelayUntil(Time + TimeStart);      /* tick = 1000Hz，Time 毫秒 = Time tick */
}

void LED_Off(void) {
    Bsp_Led_Off();
}
