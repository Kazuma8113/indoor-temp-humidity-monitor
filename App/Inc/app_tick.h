#ifndef APP_TICK_H
#define APP_TICK_H

#include "cmsis_os.h"

/* ============================================================================
 * 任务节拍（App 层）
 *
 * TIM1 每 1ms 进一次更新中断，按 100 / 100 / 1000 / 200 分频，释放对应的
 * 二值信号量把任务唤醒。任务侧直接 osSemaphoreAcquire(Sem2_xxx, osWaitForever)，
 * 没轮到自己时彻底阻塞、不占 CPU。
 *
 * 4 个信号量在这里定义、在 Core/Src/freertos.c 的 RTOS_SEMAPHORES 段创建：
 * 必须早于任何任务创建，所以只能放在那个位置，不能等中断里再建。
 * ========================================================================== */

extern osSemaphoreId_t Sem2_LED;        /* 100ms */
extern osSemaphoreId_t Sem2_LCD;        /* 100ms */
extern osSemaphoreId_t Sem2_DHT11;      /* 1000ms（传感器本身就这么慢） */
extern osSemaphoreId_t Sem2_Bluetooth;  /* 200ms */

void App_Sem_Init(void);
/* 由 TIM1 的更新中断（1ms）调用，见 Core/Src/main.c 的 HAL_TIM_PeriodElapsedCallback */
void App_Heartbeat(void);

#endif /* APP_TICK_H */
