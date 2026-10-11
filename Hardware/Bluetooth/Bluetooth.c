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

void Bluetooth_ReceiveHandler(SensorData_t *data) {
    if (!RxEnd_Flag) return;
    RxEnd_Flag = 0;

    // 判断数据是否完整
    if (Receive_Buf_Size < 3) return;
    if (Receive_Buf1[0] != '[') return;
    if (Receive_Buf1[Receive_Buf_Size - 1] != ']') return;

    // 首次连接，初始化数据，
    // 防止他一直初始化连接，加标志位判断是否初始化过
    if (Bluetooth_Link_Init(data)) return;
    // 取出主要数据
    Bluetooth_GetData(data);
}

uint8_t Bluetooth_Link_Init(SensorData_t *data) {
    static uint8_t Flag = 0;

    if (Flag == 2) return 0;

    char temp[64];
    uint16_t len = 0;

    /* 第一步：手机主动发起握手。
     * 这里不再判 Flag —— 收到就回包并复位到 1。
     * 否则手机断开重连后 Flag 仍停在 2，MCU 永远不再回包，握手会卡死（只能给板子断电）。 */
    if (strstr(Receive_Buf1, "SUCCESSFUL CONNECTION") != NULL) {
        Flag = 1;
        len = sprintf(temp, "[SUCCESSFUL CONNECTION!]\n");
        Bluetooth_SendHandler((uint8_t *) temp, len);
        return 1;
    }

    /* 第二步：手机确认，MCU 回报当前阈值 */
    if (strstr(Receive_Buf1, "**SUCCESSFUL CONNECTION**") != NULL) {
        Flag = 2;
        len = sprintf(temp, "[TEMP_MIN:%u;TEMP_MAX:%u;HUM_MIN:%u;HUM_MAX:%u]\n",
                      data->Temperature_Alarm[0], data->Temperature_Alarm[1],
                      data->Humidity_Alarm[0], data->Humidity_Alarm[1]);
        Bluetooth_SendHandler((uint8_t *) temp, len);
        return 1;
    }
    return 0;
}

void Bluetooth_GetData(SensorData_t *data) {
    char temp[64];
    uint16_t len = 0;

    /* uint16_t：必须用 %hu（只写 2 字节）unsigned short */
    if (sscanf(Receive_Buf1,
               "[TEMP_MIN:%hu;TEMP_MAX:%hu;HUM_MIN:%hu;HUM_MAX:%hu]",
               &data->Temperature_Alarm[0], &data->Temperature_Alarm[1],
               &data->Humidity_Alarm[0], &data->Humidity_Alarm[1]) == 4) {
        len = sprintf(temp, "[SET OK!]\n");
    } else if (strstr(Receive_Buf1, "LED_ON") != NULL) {
        LED_State = 1;
        len = sprintf(temp, "[SET OK!]\n");
    } else if (strstr(Receive_Buf1, "LED_OFF") != NULL) {
        LED_State = 0;
        len = sprintf(temp, "[SET OK!]\n");
    } else {
        len = sprintf(temp, "[INSTRUCTION ERROR!]\n");
    }
    Bluetooth_SendHandler((uint8_t *) temp, len);
}

//
// Created by KAZUMA on 2026/10/10.
//
