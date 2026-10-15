#include "Humidifier.h"

GPIO_PinStruct Humid_Pin = {GPIOB, GPIO_PIN_7};
GPIO_PinStruct Humid_GND = {GPIOB, GPIO_PIN_8};

void Humidifier_Set(GPIO_PinState State) {
    HAL_GPIO_WritePin(Humid_Pin.Port, Humid_Pin.Pin, State);
}



//
// Created by KAZUMA on 2026/10/10.
//
