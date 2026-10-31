#include "bsp_led.h"
#include "bsp_pin.h"

void Bsp_Led_Init(void) {
    Bsp_Led_Off();   /* 上电默认熄灭 */
}

void Bsp_Led_On(void) {
    HAL_GPIO_WritePin(LedPin.Port, LedPin.Pin, GPIO_PIN_RESET);
}

void Bsp_Led_Off(void) {
    HAL_GPIO_WritePin(LedPin.Port, LedPin.Pin, GPIO_PIN_SET);
}

void Bsp_Led_Toggle(void) {
    HAL_GPIO_TogglePin(LedPin.Port, LedPin.Pin);
}
