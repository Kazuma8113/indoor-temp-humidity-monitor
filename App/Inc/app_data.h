#ifndef APP_DATA_H
#define APP_DATA_H

#include <stdint.h>
#include "cmsis_os.h"

/* ==================== 业务数据模型 ==================== */

typedef struct {
    uint8_t  Temperature;           /* 摄氏度，DHT11 只有整数精度 */
    int16_t  Temperature_Alarm[4];  /* [0]=下限 [1]=上限（[2][3] 预留）。有符号：APP 允许 -40~85 */
    uint8_t  Humidity;              /* %RH */
    int16_t  Humidity_Alarm[4];     /* [0]=下限 [1]=上限（[2][3] 预留） */
} SensorData_t;

/* ==================== 事件标志位 ====================
 * 每位只对应一个消费者，避免多个任务等待同一位时被抢先清零。 */
#define EVENT_LCD_UPDATE    (1U << 0)   /* LCD 刷新 */
#define EVENT_BLUE_UPDATE   (1U << 1)   /* 蓝牙上报 */
#define EVENT_HUMI_UPDATE   (1U << 2)   /* 加湿控制 */
#define EVENT_WARM_UPDATE   (1U << 3)   /* 加热控制 */
#define EVENT_FAN_UPDATE    (1U << 4)   /* 风扇控制 */

/* DHT11 是唯一的数据源（MQ2 已移除），每轮读取成功后就一次性通知所有消费者。 */
#define EVENT_DATA_COMPLETE    (EVENT_LCD_UPDATE | EVENT_BLUE_UPDATE | \
                                EVENT_HUMI_UPDATE | EVENT_WARM_UPDATE | \
                                EVENT_FAN_UPDATE)

/* ==================== 共享数据 ====================
 * 读写规则：任何一次读/写都必须持有 Mutex_SensorData；写完后用事件标志通知消费者。 */
extern SensorData_t SensorData;
extern osMutexId_t Mutex_SensorData;
extern osEventFlagsId_t EF_SensorData;


/* 创建本层的 RTOS 对象（互斥锁 + 事件标志组）。
 * 在 MX_FREERTOS_Init() 的 RTOS_MUTEX 段里调一次，必须早于任何任务启动。 */
void App_Data_Init(void);

#endif /* APP_DATA_H */
