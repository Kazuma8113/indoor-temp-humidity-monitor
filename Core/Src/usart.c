/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
volatile uint8_t RxEnd_Flag, TxEnd_Flag = 1;
volatile uint16_t RxBuff_New, RxBuff_Old;
volatile uint16_t RxBuff_Size, Receive_Buf_Size;
char Rx_Buf1[UART_BUFF_SIZE], Receive_Buf1[UART_BUFF_SIZE];
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA2_Stream2;
    hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
void Usart1_Init(void) {
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);            // 使能空闲中断
    HAL_UART_Receive_DMA(&huart1, (uint8_t *) Rx_Buf1, UART_BUFF_SIZE); // 开启UART + DMA 数据接收
}

/* ==================== 接收「成帧累积」====================
 * 手机下发的一条命令（如 [TEMP_MIN:..;TEMP_MAX:..;..]）经 HC-08 会被拆成多个
 * BLE 包先后到达，每到一个包就触发一次 IDLE 中断。
 * 把每次到达的碎片追加到 Frame_Buf，直到凑出完整的 "[ ... ]" 才交给上层
 * （写到 Receive_Buf1 + 置 RxEnd_Flag）。上层 Bluetooth.c 因此完全不用改：
 * 它那两句 `Receive_Buf1[0]=='['` / `末字节==']'` 现在恒成立。
 *
 * 该函数必须放在中断里做：Bluetooth_Task 每 200ms 才轮询一次，放任务里累积会漏掉中间段数据。 */
#define UART_FRAME_MAX   128            /* 命令最长约 50 字节，128 足够 */
static char     Frame_Buf[UART_FRAME_MAX];
static uint16_t Frame_Len;

void Usart1_IDLE_Callback(void) {
    uint16_t got, i;
    char *s;
    char *e;

    // UART 在接收端检测到至少 1 个字节时间没有新数据到来
    if ((__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) == RESET)) return;
    __HAL_UART_CLEAR_IDLEFLAG(&huart1); // 清除标志位

    // 总计数减去未传输的数据个数，得到已经接收的数据个数
    RxBuff_New = UART_BUFF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
    // 没有新数据不处理（也防止刚好接收满一圈时 RxBuff_New==RxBuff_Old）
    if (RxBuff_New == RxBuff_Old) return;

    /* ---------- ① 把本次到达的这一段追加进成帧缓冲 ---------- */
    got = (RxBuff_New > RxBuff_Old)
              ? (uint16_t) (RxBuff_New - RxBuff_Old)                    // 未回绕
              : (uint16_t) (UART_BUFF_SIZE - RxBuff_Old + RxBuff_New);  // DMA 回绕
    RxBuff_Size = got;

    for (i = 0; i < got; i++) {
        char c = Rx_Buf1[RxBuff_Old];
        RxBuff_Old = (uint16_t) ((RxBuff_Old + 1U) % UART_BUFF_SIZE);

        if (Frame_Len >= UART_FRAME_MAX - 1U) {
            Frame_Len = 0;              // 一直等不到 ']'：整帧丢弃重来，避免死等
        }
        Frame_Buf[Frame_Len++] = c;
    }
    Frame_Buf[Frame_Len] = '\0';

    /* ---------- ② 丢掉第一个 '[' 之前的内容（链路噪声） ---------- */
    s = strchr(Frame_Buf, '[');
    if (s == NULL) {
        Frame_Len = 0;                  // 连帧头都没有，整段丢弃
        return;
    }
    if (s != Frame_Buf) {
        Frame_Len = (uint16_t) (Frame_Len - (uint16_t) (s - Frame_Buf));
        memmove(Frame_Buf, s, (size_t) Frame_Len + 1U);
    }

    /* ---------- ③ 找帧尾；没有说明帧还没收全，等下一段 ---------- */
    e = strchr(Frame_Buf, ']');
    if (e == NULL) return;

    /* ---------- ④ 成帧：交给上层 ---------- */
    {
        uint16_t flen = (uint16_t) (e - Frame_Buf) + 1U;   // 含 ']'
        uint16_t rest;

        /* 上一帧还没被 Bluetooth_Task 取走时先不覆盖（否则会丢那一帧） */
        if (RxEnd_Flag == 0) {
            memcpy(Receive_Buf1, Frame_Buf, flen);
            Receive_Buf1[flen] = '\0';
            Receive_Buf_Size = flen;
            RxEnd_Flag = 1;             // 现在表示：有一整帧待取
        }

        /* 把 ']' 之后多出来的字节前移，留给下一帧（支持一段里含多帧） */
        rest = (uint16_t) (Frame_Len - flen);
        memmove(Frame_Buf, e + 1, rest);
        Frame_Len = rest;
    }
}
/* USER CODE END 1 */
