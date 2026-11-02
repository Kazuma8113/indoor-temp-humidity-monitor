#include "bsp_iwdg.h"
#include "iwdg.h"      /* BSP 层可以依赖 CubeMX 生成的外设句柄 */

/* 独立看门狗喂狗函数：喂一次狗，把计数器重装回 Reload 值。*/
void Bsp_Iwdg_Feed(void) {
    HAL_IWDG_Refresh(&hiwdg);
}

/* 复位原因查询函数：查询本次复位是不是独立看门狗触发的。
 * 返回值：1 = 是，0 = 不是。
 * 注意：该标志是"粘住"的，会一直保持到被清除，所以必须在启动早期读。 */
uint8_t Bsp_Iwdg_IsResetFromWatchdog(void) {
    return (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0U) ? 1U : 0U;
}

/* 复位标志清除函数
 * 读完 Bsp_Iwdg_IsResetFromWatchdog() 之后调用，否则下次上电会读到旧标志。 */
void Bsp_Iwdg_ClearResetFlags(void) {
    __HAL_RCC_CLEAR_RESET_FLAGS();
}
