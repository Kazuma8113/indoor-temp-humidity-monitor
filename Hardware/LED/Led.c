#include "Led.h"
#include "cmsis_os2.h"

GPIO_PinStruct LED_PC13 = {GPIOC, GPIO_PIN_13};
uint8_t LED_State=1;
void LED_Flip(GPIO_PinStruct GPIO, uint32_t Time) {
    uint32_t tick = osKernelGetTickCount();

    GPIO_PinState State = HAL_GPIO_ReadPin(GPIO.Port, GPIO.Pin);
    if(State==GPIO_PIN_RESET){
        HAL_GPIO_WritePin(GPIO.Port, GPIO.Pin,GPIO_PIN_SET);
    }else{
        HAL_GPIO_WritePin(GPIO.Port, GPIO.Pin,GPIO_PIN_RESET);
    }

    osDelayUntil(tick+Time);
}

void LED_Off(GPIO_PinStruct GPIO) {
    HAL_GPIO_WritePin(GPIO.Port, GPIO.Pin,GPIO_PIN_SET);
}
