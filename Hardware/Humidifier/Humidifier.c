#include "Humidifier.h"

GPIO_PinStruct Humidifier = {GPIOB, GPIO_PIN_1};

void Humidifier_Set(GPIO_PinState State) {
    HAL_GPIO_WritePin(Humidifier.Port, Humidifier.Pin, State);
}



//
// Created by KAZUMA on 2026/10/10.
//
