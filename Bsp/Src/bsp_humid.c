#include "bsp_humid.h"
#include "bsp_pin.h"

/* 控制脚高电平开；另一根脚 Humid.GND 由 CubeMX 配成输出低，用作对接的 GND。 */
void Bsp_Humid_Init(void) {
    Bsp_Humid_Off();
}

void Bsp_Humid_On(void) {
    HAL_GPIO_WritePin(Humid.CTRL.Port, Humid.CTRL.Pin, GPIO_PIN_SET);
}

void Bsp_Humid_Off(void) {
    HAL_GPIO_WritePin(Humid.CTRL.Port, Humid.CTRL.Pin, GPIO_PIN_RESET);
}
