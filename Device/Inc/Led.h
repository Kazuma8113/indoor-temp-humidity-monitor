#ifndef DEVICE_LED_H
#define DEVICE_LED_H

#include <stdint.h>     /* uint32_t */

/* ============================================================================
 * 器件驱动：板载 LED（PC13，低电平点亮）
 * ========================================================================== */

void LED_Init(void);
void LED_Flip(uint16_t flip_time, uint16_t *now_time);
void LED_Off(void);

#endif /* DEVICE_LED_H */
