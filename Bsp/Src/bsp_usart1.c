#include "bsp_usart1.h"
#include "usart.h"      /* BSP 层可以依赖 CubeMX 生成的外设句柄 */


/* ---- 接收：DMA 目标缓冲 + 读写位置 ----
 * wr_pos 由 DMA 剩余计数反推（DMA 自己改不了我们的变量，所以"写位置"是算出来的），
 * rd_pos 是本模块取走数据后推进的。两者都在 0..BSP_USART1_BUF_SIZE-1 之间。 */
static uint8_t rx_buff[BSP_USART1_BUF_SIZE];
static volatile uint16_t wr_pos;
static volatile uint16_t rd_pos;

/* ---- 发送：空闲标志。1 = 可发，0 = DMA 正在搬 ---- */
static volatile uint8_t tx_idle = 1U;

void Bsp_Usart1_Init(void) {
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);                    /* 打开空闲中断 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_ERR);                     /* 必须显式打开错误中断 */
    HAL_UART_Receive_DMA(&huart1, rx_buff, BSP_USART1_BUF_SIZE);    /* 环形 DMA 接收 */
}

/* ==================== 发送 ==================== */
uint8_t Bsp_Usart1_SendDma(const uint8_t *buf, uint16_t len) {
    // 越界判断
    if (buf == NULL || len == 0U || len > BSP_USART1_BUF_SIZE) return 0;
    if (tx_idle == 0U) return 0;               /* 上一帧没发完，不排队 */
    tx_idle = 0U;

    if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *) buf, len) != HAL_OK) {
        tx_idle = 1U;                           /* 启动失败要恢复，否则再也发不出去 */
        return 0;
    }
    return 1;  // 发送成功
}

// 判断Tx线是否空闲
uint8_t Bsp_Usart1_IsTxDone(void) {
    return (tx_idle == 1);
}

// 用 static 限制变量访问范围，只能通过函数访问（实现私有变量）
void Bsp_Usart1_Tx_IDLE(void) { tx_idle = 1; }

// usart1 DMA 接收错误处理
void Bsp_Usart1_OnError(void) {
    /* 清掉所有可能的错误标志 */
    __HAL_UART_CLEAR_OREFLAG(&huart1);    /* RDR 过载 */
    __HAL_UART_CLEAR_PEFLAG(&huart1);     /* 奇偶校验 */
    __HAL_UART_CLEAR_FEFLAG(&huart1);     /* 停止位 */
    __HAL_UART_CLEAR_NEFLAG(&huart1);     /* 噪声 */

    Bsp_Usart1_Tx_IDLE();                  /* 恢复发送空闲标志位 */

    /* 重启 DMA 接收（错误可能导致接收停摆）。
     * 同时把读写指针归零：DMA 重新从缓冲头部开始写，旧位置上的残留不能再用，
     * 否则下一次 take_bytes 会把上一次的垃圾当成新字节交出去。 */
    HAL_UART_Receive_DMA(&huart1, rx_buff, BSP_USART1_BUF_SIZE);
    rd_pos = 0U;
    wr_pos = 0U;
}

/* ==================== 接收 ==================== */
// 计算这一次接收字节个数
static uint16_t RxBuff_Len(void) {
    uint16_t wr = wr_pos;
    uint16_t rd = rd_pos;

    /* 回绕：写位置跑到读位置前面去了，说明绕了一圈 */
    return (wr >= rd) ? (uint32_t) (wr - rd)
                      : (uint32_t) (BSP_USART1_BUF_SIZE - rd + wr);
}

/* 取出字节 */
uint8_t Bsp_Usart1_TakeBytes(uint8_t *buff, uint16_t *len) {

    if (buff == NULL || *len == 0U) return 0;

    *len = RxBuff_Len();

    /* 缓冲区中没有字节 */
    if (*len == 0U) {
        /* 先判断有没有来新数据，没有产生 IDLE 中断，则无需判断数据 */
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) == RESET) return 0;
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);  // 清除 IDLE 标志位

        /* 发生了 IDLE 中断，有两种情况：
         * 1. 首次 DMA 接收完成，读写指针都是0，还未初始化 */
        // 写指针不用自增，因为DMA退回的数据长度本身就是累积值
        wr_pos = (uint16_t) (BSP_USART1_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx));
        // 本次接收数据的长度刚好是 BSP_USART1_BUF_SIZE
        if (wr_pos >= BSP_USART1_BUF_SIZE) wr_pos = 0U;
        // 初始化完写指针，再次判断接收字节长度
        *len = RxBuff_Len();

        /* 2. 之后接收数据长度 == BSP_USART1_BUF_SIZE */
        if (*len == 0U) *len = BSP_USART1_BUF_SIZE;
    }

    /* 按环形顺序拷出去 */
    for (uint16_t i = 0; i < *len; i++) {
        buff[i] = rx_buff[rd_pos];
        rd_pos = (rd_pos + 1) % BSP_USART1_BUF_SIZE;
    }

    return 1;
}
