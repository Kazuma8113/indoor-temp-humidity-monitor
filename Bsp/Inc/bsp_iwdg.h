#ifndef BSP_IWDG_H
#define BSP_IWDG_H

#include <stdint.h>


void Bsp_Iwdg_Feed(void);
uint8_t Bsp_Iwdg_IsResetFromWatchdog(void);
void Bsp_Iwdg_ClearResetFlags(void);

#endif /* BSP_IWDG_H */
