#include "Warm.h"

GPIO_PinStruct Warm_Pin = {GPIOD, GPIO_PIN_10};

void Warm_Set(GPIO_PinState State) {
    HAL_GPIO_WritePin(Warm_Pin.Port, Warm_Pin.Pin, State);
}

