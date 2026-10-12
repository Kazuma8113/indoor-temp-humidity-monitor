#ifndef __BLUETOOTH_H__
#define __BLUETOOTH_H__

#include "main.h"
#include "usart.h"

/* 共享数据：所有任务通过它交换传感器数据。
 * 读写规则：任何一次读/写都必须持有 Mutex_SensorData；写完后用事件标志通知消费者。 */
typedef struct {
    uint8_t Temperature;
    uint16_t Temperature_Alarm[4];
    uint8_t Humidity;
    uint16_t Humidity_Alarm[4];
} SensorData_t;

typedef enum {
    BT_STATUS_BUSY,              // 串口接收繁忙
    BT_STATUS_SUCCESS,           // 数据接收并解析成功
    BT_STATUS_HANDSHAKE,         // 握手包，已完成应答
    BT_STATUS_FRAME_ERROR,       // 数据帧格式错误
    BT_STATUS_DATA_TOO_SHORT,    // 数据长度不足
    BT_STATUS_PARSE_ERROR        // 数据解析失败
} Bluetooth_Status_t;

/* 解析出的命令种类：调用方据此决定要不要把 data 写回共享区 SensorData。
 * 需要区分是因为：阈值包要写 SensorData（得加锁），
 * 而 LED 指令只改 LED_State（LED_State_Set 内部已经自己加锁了）。 */
typedef enum {
    BT_CMD_NONE = 0,      /* 什么都没解析出来 */
    BT_CMD_THRESHOLD,     /* 阈值包：data 的 4 个 Alarm 字段有效，需要写回 SensorData */
    BT_CMD_LED            /* LED 指令：已直接作用到 LED_State，不需要写回 SensorData */
} Bluetooth_CmdKind_t;

void Bluetooth_SendHandler(uint8_t *Buff, uint16_t len);
Bluetooth_Status_t Bluetooth_ReceiveHandler(SensorData_t *data, Bluetooth_CmdKind_t *kind);
uint8_t Bluetooth_Link_Init(void);
Bluetooth_Status_t Bluetooth_GetData(SensorData_t *data, Bluetooth_CmdKind_t *kind);
void Bluetooth_ReplyByStatus(Bluetooth_Status_t status);

#endif //PROJECT_BLUETOOTH_H
//
// Created by KAZUMA on 2026/10/10.
//
