#ifndef __DHT11_H
#define __DHT11_H

#include <stdint.h>

#include "bsp_dht11.h"    /* 单总线引脚：读 / 写 / 切方向（Bsp 直调 HAL） */

/* ============================================================================
 * DHT11 单总线温湿度传感器
 *
 * 本驱动只管时序（起始信号、应答检测、40 位读取、校验），
 * ========================================================================== */

/* 控制数据线电平 */
#define DHT11_Low    Bsp_Dht11_Write(0)
#define DHT11_High   Bsp_Dht11_Write(1)

uint8_t DHT11_Init(void);                              /* 初始化引脚并检测器件：0=存在 */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi); /* 读一次温湿度：0=成功 */
uint8_t DHT11_Read_Byte(void);                         /* 读一个字节 */
uint8_t DHT11_Read_Bit(void);                          /* 读一位 */
uint8_t DHT11_Check(void);                             /* 检测应答：0=存在 */
void    DHT11_Rst(void);                               /* 发起始信号 */

#endif
