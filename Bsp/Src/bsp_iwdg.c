#include "bsp_iwdg.h"
#include "iwdg.h"      /* BSP 层可以依赖 CubeMX 生成的外设句柄 */

void Bsp_Iwdg_Feed(void) {
    HAL_IWDG_Refresh(&hiwdg);
}
