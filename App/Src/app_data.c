#include "app_data.h"
#include "app_led.h"    /* LED 业务状态机（已从 Device 提到 App） */

/* ==================== 全局唯一的共享数据实例 ==================== */
SensorData_t SensorData = {
        .Temperature = 0,
        .Temperature_Alarm = {0, 99},
        .Humidity = 0,
        .Humidity_Alarm = {0, 99}
};

static const osMutexAttr_t s_mutex_attr = {
        .attr_bits = osMutexRecursive | osMutexPrioInherit
};
osMutexId_t Mutex_SensorData;

/* 事件标志组：通知消费者"数据已就绪" */
osEventFlagsId_t EF_SensorData;

void App_Data_Init(void) {
    Mutex_SensorData = osMutexNew(&s_mutex_attr);
    EF_SensorData    = osEventFlagsNew(NULL);

    /* LED 状态机的互斥锁在 app_led.c 内部自管，这里负责触发创建 */
    App_Led_Init();
}
