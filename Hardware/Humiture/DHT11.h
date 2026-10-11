#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

extern GPIO_PinStruct DHT11;


// 时钟使能宏（HAL库方式）
#define DHT11_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
/*********************END**********************/

// 引脚模式定义
#define OUT  1   // 输出模式
#define IN   0   // 输入模式

// 控制引脚输出高低电平（HAL库方式）
#define DHT11_Low   HAL_GPIO_WritePin(DHT11.Port, DHT11.Pin, GPIO_PIN_RESET)
#define DHT11_High  HAL_GPIO_WritePin(DHT11.Port, DHT11.Pin, GPIO_PIN_SET)

// 函数声明（返回类型使用HAL库标准类型 uint8_t）
uint8_t DHT11_Init(void);                // 初始化DHT11
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi); // 读取温湿度数据
uint8_t DHT11_Read_Byte(void);           // 读取一个字节
uint8_t DHT11_Read_Bit(void);            // 读取一位
void    DHT11_Mode(uint8_t mode);        // 设置引脚模式
uint8_t DHT11_Check(void);               // 检测DHT11是否存在
void    DHT11_Rst(void);                 // 复位DHT11

#endif