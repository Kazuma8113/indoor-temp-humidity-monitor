#ifndef BSP_USART1_H
#define BSP_USART1_H

#include <stdint.h>

#define BSP_USART1_BUF_SIZE   128U

/* 初始化：启动 DMA 接收 + 打开 IDLE / 错误中断。上电调用一次。*/
void Bsp_Usart1_Init(void);

/* ---- 发送数据 ---- */
/* 异步发送（DMA）。返回 1 = 已启动，0 = 失败。
 * 失败有三种原因（参数非法 / 上一帧还没发完 / HAL 启动失败），本层不再细分。 */
uint8_t Bsp_Usart1_SendDma(const uint8_t *buf, uint16_t len);

/* 上一帧发完了吗：1 = 空闲可发，0 = 仍在发送。*/
uint8_t Bsp_Usart1_IsTxDone(void);

/* ---- 接收数据 ---- */
uint8_t Bsp_Usart1_TakeBytes(uint8_t *buff, uint16_t *len);

/* ---- 给 HAL 回调用的两个入口 ---- */
void Bsp_Usart1_Tx_IDLE(void);  /* 发送完成：HAL_UART_TxCpltCallback 里调用 */
void Bsp_Usart1_OnError(void);  /* 串口出错：HAL_UART_ErrorCallback 里调用。*/

#endif /* BSP_USART1_H */
