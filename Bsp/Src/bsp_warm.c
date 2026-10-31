#include "bsp_warm.h"
#include "bsp_pin.h"

/*  加热片低电平有效：On = RESET，Off = SET。极性只写在这里。 */

void Bsp_Warm_Init(void) {
    Bsp_Warm_Off();
}

void Bsp_Warm_On(void) {
    HAL_GPIO_WritePin(WarmPin.Port, WarmPin.Pin, GPIO_PIN_RESET);
}

void Bsp_Warm_Off(void) {
    HAL_GPIO_WritePin(WarmPin.Port, WarmPin.Pin, GPIO_PIN_SET);
}
