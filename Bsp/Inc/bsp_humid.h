#ifndef BSP_HUMID_H
#define BSP_HUMID_H

/* ============================================================================
 * 板级封装：加湿器（PF5 控制，PF6 对接 GND）
 *
 * 控制脚高电平开。极性封在本层，上层只说 On / Off。
 * ========================================================================== */

void Bsp_Humid_Init(void);    /* 初始化为关闭（输出低） */
void Bsp_Humid_On(void);
void Bsp_Humid_Off(void);

#endif /* BSP_HUMID_H */
