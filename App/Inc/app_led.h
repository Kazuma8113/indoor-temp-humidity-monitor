#ifndef APP_LED_H
#define APP_LED_H

#include <stdint.h>

/* ============================================================================
 * LED 业务状态机（App 层）
 * ========================================================================== */

#define LED_STATE_OFF         0U   /* 常灭 */
#define LED_STATE_ON          1U   /* 0.5s 一翻转（正常运行心跳） */
#define LED_STATE_DHT11_FAIL  2U   /* DHT11 初始化失败：0.2s 快闪 */

/* 上电默认 ON：0.5s 心跳一眼就能看出"系统在跑" */
void    App_Led_Init(void);
void    App_Led_SetState(uint8_t state);
uint8_t App_Led_GetState(void);

#endif /* APP_LED_H */
