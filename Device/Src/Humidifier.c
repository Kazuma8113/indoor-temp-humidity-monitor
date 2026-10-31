#include "Humidifier.h"
#include "bsp_humid.h"    /* Bsp_Humid_On / Off（HAL 直调，极性在 Bsp 里） */

void Humidifier_Init(void) {
    Bsp_Humid_Init();
}

void Humidifier_Set(uint8_t on) {
    if (on) {
        Bsp_Humid_On();
    } else {
        Bsp_Humid_Off();
    }
}
