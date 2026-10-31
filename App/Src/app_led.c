#include "app_led.h"

#include "Led.h"        /* LED_Init()：把灯先置为熄灭 */
#include "cmsis_os.h"

/* 上电默认：0.5s 心跳，表示系统在跑 */
static uint8_t led_state = LED_STATE_ON;

static const osMutexAttr_t s_mutex_attr = {
        .attr_bits = osMutexRecursive | osMutexPrioInherit
};
static osMutexId_t led_mutex = NULL;

void App_Led_Init(void) {
    led_state = LED_STATE_ON;
    led_mutex = osMutexNew(&s_mutex_attr);
    LED_Init();                 /* 引脚初始为熄灭，然后交给心跳状态机 */
}

void App_Led_SetState(uint8_t state) {
    if (led_mutex != NULL &&
        osMutexAcquire(led_mutex, osWaitForever) == osOK) {
        led_state = state;
        osMutexRelease(led_mutex);
    } else {
        led_state = state;   /* 兜底：锁还没建好（极罕见），单字节写本身是原子的 */
    }
}

uint8_t App_Led_GetState(void) {
    uint8_t state = led_state;   /* 兜底值：拿不到锁时退回无保护读（单字节读原子） */

    if (led_mutex != NULL &&
        osMutexAcquire(led_mutex, osWaitForever) == osOK) {
        state = led_state;
        osMutexRelease(led_mutex);
    }
    return state;
}
