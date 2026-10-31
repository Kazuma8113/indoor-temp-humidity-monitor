#include "Warm.h"
#include "bsp_warm.h"     /* Bsp_Warm_On / Off（"低电平有效"在 Bsp 里） */

void Warm_Init(void) {
    Bsp_Warm_Init();
}

void Warm_Set(uint8_t on) {
    if (on) {
        Bsp_Warm_On();
    } else {
        Bsp_Warm_Off();
    }
}
