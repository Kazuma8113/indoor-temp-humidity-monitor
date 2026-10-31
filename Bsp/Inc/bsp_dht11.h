#ifndef BSP_DHT11_H
#define BSP_DHT11_H

#include <stdint.h>

/* ============================================================================
 * 板级封装：DHT11 的单总线数据脚（PA11）
 *
 * 只提供"这根脚怎么读写、怎么切方向"，不含任何 1-Wire 时序
 * 协议时序在 Device/Src/DHT11.c 里（它才是 DHT11 的驱动）。
 *
 * 单总线需要"输出/上拉输入"来回切，所以模式切换必须由本层提供：
 * 上层不能直接填 GPIO_InitTypeDef。
 * ========================================================================== */

void    Bsp_Dht11_Init(void);              /* 配成推挽输出，空闲拉高 */
void    Bsp_Dht11_SetOutput(void);         /* 切到推挽输出（主机发起始信号） */
void    Bsp_Dht11_SetInputPullup(void);    /* 切到上拉输入（读传感器） */
void    Bsp_Dht11_Write(uint8_t level);    /* level = 1 拉高，否则拉低 */
uint8_t Bsp_Dht11_Read(void);              /* 1 = 高电平，0 = 低电平 */

#endif /* BSP_DHT11_H */
