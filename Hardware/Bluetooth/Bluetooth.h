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


void Bluetooth_SendHandler(uint8_t *Buff, uint16_t len);

void Bluetooth_ReceiveHandler(SensorData_t *data);
uint8_t Bluetooth_Link_Init(SensorData_t *data);
void Bluetooth_GetData(SensorData_t *data);
#endif //PROJECT_BLUETOOTH_H
//
// Created by KAZUMA on 2026/10/10.
//
