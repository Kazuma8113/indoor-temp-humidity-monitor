#ifndef BSP_LED_H
#define BSP_LED_H

/* ============================================================================
 * 板级封装：板载 LED（PC13，低电平点亮）
 *
 * "低电平点亮"这个硬件特性封在 bsp_led.c 里，上层只说 On / Off / Toggle。
 * ========================================================================== */

void Bsp_Led_Init(void);      /* 初始化为熄灭（输出高） */
void Bsp_Led_On(void);        /* 点亮 */
void Bsp_Led_Off(void);       /* 熄灭 */
void Bsp_Led_Toggle(void);    /* 翻转 */

#endif /* BSP_LED_H */
