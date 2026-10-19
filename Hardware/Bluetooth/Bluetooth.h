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
    BT_DHT_STATUS_SUCCESS,       // 温湿度数据接收并解析成功
    BT_LED_STATUS_SUCCESS,       // LED 指令接收并解析成功
    BT_STATUS_HANDSHAKE,         // 握手包，已完成应答
    BT_STATUS_FRAME_ERROR,       // 数据帧格式错误
    BT_STATUS_DATA_TOO_SHORT,    // 数据长度不足
    BT_STATUS_PARSE_ERROR        // 数据解析失败
} Bluetooth_Status_t;


void Bluetooth_SendHandler(uint8_t *Buff, uint16_t len);
Bluetooth_Status_t Bluetooth_ReceiveHandler(SensorData_t *data);
uint8_t Bluetooth_Handshake(void);
Bluetooth_Status_t Bluetooth_GetData(SensorData_t *data);
void Bluetooth_ReplyByStatus(Bluetooth_Status_t status);

#endif //PROJECT_BLUETOOTH_H
//
// Created by KAZUMA on 2026/10/10.
//
