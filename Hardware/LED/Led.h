#ifndef GRADUATION_PROJECT_LED_H
#define GRADUATION_PROJECT_LED_H

#include "main.h"


extern GPIO_PinStruct LED_PC13;
extern uint8_t LED_State;

void LED_Flip(GPIO_PinStruct GPIO, uint32_t Time);
void LED_Off(GPIO_PinStruct GPIO);

#endif //GRADUATION_PROJECT_LED_H
