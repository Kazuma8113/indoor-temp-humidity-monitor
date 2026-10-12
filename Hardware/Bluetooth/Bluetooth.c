#include "Bluetooth.h"
#include <stdio.h>
#include "Led.h"
#include "cmsis_os.h"   /* Bluetooth_SendHandler 里用 osDelay / osKernelGetTickCount */

//发送处理函数
void Bluetooth_SendHandler(uint8_t *Buff, uint16_t len) {
    /* 使用静态存储，防止函数结束，DMA获取错误数据 */
    static uint8_t tx_buf[UART_BUFF_SIZE];
    uint32_t start;

    if (len == 0 || len >= UART_BUFF_SIZE) return;

    /* 发送线路忙时短暂等待，而不是直接丢弃。
     * 一帧上报 @9600 只占二十几 ms；本函数只在任务上下文被调用（Bluetooth_Task），
     * 所以可以用 osDelay 让出 CPU 等它发完。 */
    start = osKernelGetTickCount();
    while (TxEnd_Flag == 0) {
        if ((osKernelGetTickCount() - start) > 100U) return;   /* 超时放弃，防止卡死 */
        osDelay(1);
    }

    TxEnd_Flag = 0;
    memcpy(tx_buf, Buff, len);
    if (HAL_UART_Transmit_DMA(&huart1, tx_buf, len) != HAL_OK) {
        TxEnd_Flag = 1;   /* 启动失败就恢复标志，避免之后再也发不出去 */
    }
}

Bluetooth_Status_t Bluetooth_ReceiveHandler(SensorData_t *data, Bluetooth_CmdKind_t *kind) {
    if (kind != NULL) *kind = BT_CMD_NONE;
    if (!RxEnd_Flag) return BT_STATUS_BUSY;
    RxEnd_Flag = 0;

    /* 帧完整性：长度 / 首字节 '[' / 末字节 ']' */
    if (Receive_Buf_Size < 3) return BT_STATUS_DATA_TOO_SHORT;
    if (Receive_Buf1[0] != '[') return BT_STATUS_FRAME_ERROR;
    if (Receive_Buf1[Receive_Buf_Size - 1] != ']') return BT_STATUS_FRAME_ERROR;

    /* 握手包：到此为止，不能再往下当普通指令解析
     * （否则 sscanf 匹配不上，会跟着多发一条错误应答） */
    if (Bluetooth_Link_Init()) return BT_STATUS_HANDSHAKE;

    /* 普通指令：解析结果写进调用方给的局部副本，状态由调用方决定怎么回话 */
    return Bluetooth_GetData(data, kind);
}

/* 一次性握手：手机发 [SUCCESSFUL CONNECTION!] → MCU 回一条即完成，没有第二次握手。
 * 本函数只做“这包是不是握手包”的判断，回包交给 Bluetooth_ReplyByStatus(BT_STATUS_HANDSHAKE)。
 * 刻意不加“只回一次”的静态标志：手机断线重连后那样就再也不会回包，握手会卡死（只能给板子断电）；
 * 而回包本身是幂等的，手机重发一次握手也不会造成什么后果。 */
uint8_t Bluetooth_Link_Init(void) {
    return (strstr(Receive_Buf1, "SUCCESSFUL CONNECTION") != NULL) ? 1U : 0U;
}

/* 只解析、返回结果，不发包；应答统一由 Bluetooth_ReplyByStatus() 发。
 * 解析结果写进 data —— 那是调用方传进来的**局部副本**，共享区 SensorData
 * 由调用方自己决定什么时候加锁写回（见 Bluetooth_StartTask 的三段式）。
 * uint16_t：必须用 %hu（只写 2 字节）unsigned short */
Bluetooth_Status_t Bluetooth_GetData(SensorData_t *data, Bluetooth_CmdKind_t *kind) {
    if (kind != NULL) *kind = BT_CMD_NONE;

    /* 阈值包：只落进局部副本，不在这里碰共享区 */
    if (sscanf(Receive_Buf1,
               "[TEMP_MIN:%hu;TEMP_MAX:%hu;HUM_MIN:%hu;HUM_MAX:%hu]",
               &data->Temperature_Alarm[0], &data->Temperature_Alarm[1],
               &data->Humidity_Alarm[0], &data->Humidity_Alarm[1]) == 4) {
        if (kind != NULL) *kind = BT_CMD_THRESHOLD;
        return BT_STATUS_SUCCESS;
    }

    if (strstr(Receive_Buf1, "LED_ON") != NULL) {
        LED_State_Set(1);            /* 内部自带 Mutex_LedState，这里不用再加锁 */
        if (kind != NULL) *kind = BT_CMD_LED;
        return BT_STATUS_SUCCESS;
    }

    if (strstr(Receive_Buf1, "LED_OFF") != NULL) {
        LED_State_Set(0);
        if (kind != NULL) *kind = BT_CMD_LED;
        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_PARSE_ERROR;
}

/* 按接收处理的结果给手机回一条应答。手机靠它判断 MCU 的状态：
 *   收到 [SET OK!] / [SUCCESSFUL CONNECTION!]  -> 表示已成功处理，手机停止重发；
 *   收到 [FRAME ERROR!] / [DATA TOO SHORT!] / [PARSE ERROR!] -> 表示没处理成功，手机会重发上一条；
 *   BT_STATUS_BUSY（本轮没收到完整帧）什么都不回，否则每轮都会白发一条。 */
void Bluetooth_ReplyByStatus(Bluetooth_Status_t status) {
    char temp[64];
    uint16_t len;

    switch (status) {
        case BT_STATUS_HANDSHAKE:
            len = sprintf(temp, "[SUCCESSFUL CONNECTION!]\n");
            break;
        case BT_STATUS_SUCCESS:
            len = sprintf(temp, "[SET OK!]\n");
            break;
        case BT_STATUS_FRAME_ERROR:
            len = sprintf(temp, "[FRAME ERROR!]\n");
            break;
        case BT_STATUS_DATA_TOO_SHORT:
            len = sprintf(temp, "[DATA TOO SHORT!]\n");
            break;
        case BT_STATUS_PARSE_ERROR:
            len = sprintf(temp, "[PARSE ERROR!]\n");
            break;

        case BT_STATUS_BUSY:
        default:
            return;
    }

    Bluetooth_SendHandler((uint8_t *) temp, len);
}

//
// Created by KAZUMA on 2026/10/10.
//
