#include "app_tick.h"

/* 4 个二值信号量：在这里定义，在 Core/Src/freertos.c 的 RTOS_SEMAPHORES 段创建。 */
osSemaphoreId_t Sem2_LED;
osSemaphoreId_t Sem2_LCD;
osSemaphoreId_t Sem2_DHT11;
osSemaphoreId_t Sem2_Bluetooth;
void App_Sem_Init(void) {
    Sem2_LED       = osSemaphoreNew(1, 1, NULL);
    Sem2_LCD       = osSemaphoreNew(1, 1, NULL);
    Sem2_DHT11     = osSemaphoreNew(1, 1, NULL);
    Sem2_Bluetooth = osSemaphoreNew(1, 1, NULL);
}

/* 1ms 中断上下文（TIM1 更新中断 → HAL_TIM_PeriodElapsedCallback → 本函数）*/
void App_Heartbeat(void) {
    static uint8_t LED_Count;
    static uint8_t LCD_Count;
    static uint16_t DHT11_Count;
    static uint8_t Bluetooth_Count;

    /* 用二值信号量控制CPU资源占用，避免高优先级任务一直占用CPU
     * 计数器必须"无条件"清零：防止信号量还是满的（任务没来得及取走）时，计数值溢出*/
    if (++LED_Count >= 100) {
        LED_Count = 0;
        osSemaphoreRelease(Sem2_LED);
    }

    if (++LCD_Count >= 100) {
        LCD_Count = 0;
        osSemaphoreRelease(Sem2_LCD);
    }

    if (++DHT11_Count >= 1000) {
        DHT11_Count = 0;
        osSemaphoreRelease(Sem2_DHT11);
    }

    if (++Bluetooth_Count >= 200) {
        Bluetooth_Count = 0;
        osSemaphoreRelease(Sem2_Bluetooth);
    }
}
