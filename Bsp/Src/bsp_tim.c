#include "bsp_tim.h"
#include "tim.h"        /* BSP 层可以依赖 CubeMX 生成的外设句柄，上层不行 */

void Bsp_Tim_Start(void) {
    HAL_TIM_Base_Start_IT(&htim1);      /* 1ms 更新中断，分频由 Service/tick.c 负责 */
}
