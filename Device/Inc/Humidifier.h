#ifndef __HUMIDIFIER_H__
#define __HUMIDIFIER_H__

#include <stdint.h>     /* uint8_t */

/* ============================================================================
 * 器件驱动：加湿器（控制脚高电平开）
 *
 * 对外只说"开/关"，不说电平 —— 极性是这块板子的电气事实，属于本文件，
 * ========================================================================== */

/* 初始化（上电关闭） */
void Humidifier_Init(void);

/* on = 1 开加湿，on = 0 关 */
void Humidifier_Set(uint8_t on);

#endif //__HUMIDIFIER_H__
