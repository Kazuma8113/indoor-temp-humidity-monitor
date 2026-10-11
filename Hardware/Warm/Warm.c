#include "Warm.h"

GPIO_PinStruct Warm_Pin = {GPIOB, GPIO_PIN_4};

void Warm_Set(GPIO_PinState State) {
    HAL_GPIO_WritePin(Warm_Pin.Port, Warm_Pin.Pin, State);
}

//
// Created by KAZUMA on 2026/10/10.
//
