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

void Bluetooth_SendHandler(uint8_t *Buff, uint16_t len);

/* 接收处理：返回本次处理结果（Bluetooth_Status_t），调用方据此回话。
 * 注意：本函数内部不再自己发应答，统一交给 Bluetooth_ReplyByStatus()。 */
Bluetooth_Status_t Bluetooth_ReceiveHandler(SensorData_t *data);

/* 一次性握手：只判断"这包是不是握手包"，返回 1 = 是，调用方不要再当普通指令解析。
 * 不再自己发包；回包由 Bluetooth_ReplyByStatus(BT_STATUS_HANDSHAKE) 统一负责。 */
uint8_t Bluetooth_Link_Init(void);

Bluetooth_Status_t Bluetooth_GetData(SensorData_t *data);

/* 按接收处理结果给手机回一条应答 —— 手机靠它判断 MCU 的状态，决定要不要重发：
 *   BT_STATUS_SUCCESS   -> [SET OK!]                已成功处理，手机停止重发
 *   BT_STATUS_HANDSHAKE -> [SUCCESSFUL CONNECTION!] 握手完成，手机进入下一步
 *   BT_STATUS_FRAME_ERROR / DATA_TOO_SHORT / PARSE_ERROR
 *                       -> [FRAME ERROR!] / [DATA TOO SHORT!] / [PARSE ERROR!]
 *                          说明这条没被成功处理，手机会重发上一条
 *   BT_STATUS_BUSY      -> 不回话（本轮没收到完整帧，回了反而每轮都多发一条） */
void Bluetooth_ReplyByStatus(Bluetooth_Status_t status);

#endif //PROJECT_BLUETOOTH_H
//
// Created by KAZUMA on 2026/10/10.
//
