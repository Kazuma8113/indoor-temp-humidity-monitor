/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <string.h>
/* Hardware/LCD 的液晶驱动（lcd_spi_154.h）会 #include "usart.h"，
 * 其内部使用 sprintf 格式化数字，需要 stdio.h 的声明；此处一并转发。 */
#include <stdio.h>
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

/* USER CODE BEGIN Private defines */
#define UART_BUFF_SIZE 256

/* 蓝牙串口（USART1）的收发 DMA 句柄，定义在 usart.c */
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

extern volatile uint16_t RxBuff_Size, Receive_Buf_Size;
extern volatile uint8_t RxEnd_Flag, TxEnd_Flag;
extern volatile uint16_t RxBuff_New, RxBuff_Old;
extern char Rx_Buf1[UART_BUFF_SIZE], Receive_Buf1[UART_BUFF_SIZE];

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);

/* USER CODE BEGIN Prototypes */
/* 蓝牙串口（USART1）收发：DMA + 空闲中断 */
void Usart1_Init(void);
void Usart1_IDLE_Callback(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

