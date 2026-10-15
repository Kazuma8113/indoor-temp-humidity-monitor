#include "Led.h"
#include "cmsis_os2.h"

GPIO_PinStruct LED_PC13 = {GPIOC, GPIO_PIN_13};
uint8_t LED_State = LED_STATE_ON;   /* 上电默认：0.5s 心跳，表示系统在跑 */

/* 保护 LED_State 的互斥锁（LED_Task 读 / Bluetooth_Task 写）。
 * 属性与 Mutex_SensorData 保持一致：递归 + 优先级继承。
 * 临界区里只有一条赋值语句，不调用任何会阻塞的函数。 */
static osMutexId_t Mutex_LedState = NULL;
static const osMutexAttr_t attr_LedState = {
        .attr_bits = osMutexRecursive | osMutexPrioInherit
};

void LED_State_Init(void) {
    Mutex_LedState = osMutexNew(&attr_LedState);
}

void LED_State_Set(uint8_t state) {
    if (Mutex_LedState != NULL &&
        osMutexAcquire(Mutex_LedState, osWaitForever) == osOK) {
        LED_State = state;
        osMutexRelease(Mutex_LedState);
    } else {
        LED_State = state;   /* 兜底：锁还没建好（极罕见），单字节写本身是原子的 */
    }
}

uint8_t LED_State_Get(void) {
    uint8_t state = LED_State;   /* 兜底值：拿不到锁时退回无保护读（单字节读原子） */

    if (Mutex_LedState != NULL &&
        osMutexAcquire(Mutex_LedState, osWaitForever) == osOK) {
        state = LED_State;
        osMutexRelease(Mutex_LedState);
    }
    return state;
}

void LED_Flip(GPIO_PinStruct GPIO, uint32_t Time) {
    HAL_GPIO_TogglePin(GPIO.Port, GPIO.Pin);
    uint32_t tick = osKernelGetTickCount();
    osDelayUntil(tick + Time);
}

void LED_Off(GPIO_PinStruct GPIO) {
    HAL_GPIO_WritePin(GPIO.Port, GPIO.Pin, GPIO_PIN_SET);
}
