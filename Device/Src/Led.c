#include "led.h"
#include "bsp_led.h"      /* Bsp_Led_On / Off / Toggle / Init（HAL 直调，引脚内置） */

void LED_Init(void) {
    Bsp_Led_Init();     /* 初始化为熄灭 */
}

void LED_Flip(uint16_t flip_time, uint16_t *now_time) {
    if (*now_time * 100 >= flip_time) {
        *now_time = 0;
        Bsp_Led_Toggle();
    }
}

void LED_Off(void) {
    Bsp_Led_Off();
}
