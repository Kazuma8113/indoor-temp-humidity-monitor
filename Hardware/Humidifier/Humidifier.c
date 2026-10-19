#include "Humidifier.h"

GPIO_PinStruct Humid_Pin = {GPIOF, GPIO_PIN_5};
GPIO_PinStruct Humid_GND = {GPIOF, GPIO_PIN_6};

void Humidifier_Set(GPIO_PinState State) {
    HAL_GPIO_WritePin(Humid_Pin.Port, Humid_Pin.Pin, State);
}



//
// Created by KAZUMA on 2026/10/10.
//
