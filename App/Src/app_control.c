#include "app_control.h"
#include "app_data.h"

#include "Humidifier.h"
#include "Warm.h"
#include "TB6612.h"

#include "cmsis_os.h"

/* ==================== 风扇档位（TB6612 -> 直流马达，VM = 5V）====================
 * 5V 供电时马达有启动死区（实测约 60% 以下只嗡不转），所以最低档直接给 75 */
#define FAN_SPEED_OFF       0U     /* 停 */
#define FAN_SPEED_LOW       75U    /* 温度高出上限 0~3 ℃ */
#define FAN_SPEED_MID       85U    /* 温度高出上限 3~6 ℃  或  湿度超过上限 */
#define FAN_SPEED_HIGH      100U   /* 温度高出上限 6 ℃ 以上 */

/* 加热回差：温度顶到"下限 + TEMP_HYST"才关，跌破下限才开，中间保持原状态。 */
#define TEMP_HYST           2U

/* ==================== 湿度闭环 ====================
 * Humid_Status 必须放在循环外：靠"中间区间保持上次状态"做出天然回差，避免湿度在阈值附近抖动时加湿器反复开关。*/
void App_Humidifier_Run(void) {
    uint8_t Humid_Status = 0;

    for (;;) {
        SensorData_t data;
        /* 无限等待，不存在超时，所以返回值不用判（只有 NO_WAIT 的调用点才要挡负错误码） */
        osEventFlagsWait(EF_SensorData, EVENT_HUMI_UPDATE, osFlagsWaitAll, osWaitForever);

        if (osMutexAcquire(Mutex_SensorData, 0U) == osOK) {
            data = SensorData;
            osMutexRelease(Mutex_SensorData);

            if (data.Humidity >= data.Humidity_Alarm[1]) {
                Humid_Status = 0;   /* 顶到上限：无条件关。雾化有扩散滞后，这一条压住过冲 */
            } else if (data.Humidity <= data.Humidity_Alarm[0]) {
                Humid_Status = 1;   /* 跌破下限：开 */
            }

            Humidifier_Set(Humid_Status);   /* 1 = 开，极性由驱动内部处理 */
        }
    }
}

/* ==================== 温度闭环（加热）====================
 * Warm_Status 放在循环外：靠"回差带内保持上次状态"避免加热片频繁通断。 */
void App_Warm_Run(void) {
    uint8_t Warm_Status = 0;

    for (;;) {
        SensorData_t data;
        osEventFlagsWait(EF_SensorData, EVENT_WARM_UPDATE, osFlagsWaitAll, osWaitForever);

        if (osMutexAcquire(Mutex_SensorData, 0U) == osOK) {
            data = SensorData;
            osMutexRelease(Mutex_SensorData);

            if (data.Temperature >= data.Temperature_Alarm[0] + TEMP_HYST) {
                Warm_Status = 0;    /* 升到"下限 + 回差"：关，防止贴着下限抖动 */
            } else if (data.Temperature <= data.Temperature_Alarm[0]) {
                Warm_Status = 1;    /* 跌破下限：开 */
            }
            /* 下限 ~ 下限+TEMP_HYST 之间：保持原状态，这就是回差带 */

            /* 低电平有效：Warm_Status = 1 → 输出低电平（加热开） */
            Warm_Set(Warm_Status);   /* 1 = 开；"低电平有效"在驱动内部处理 */
        }
    }
}

/* ==================== 风扇闭环 ====================
 * 风扇要同时给两个闭环干活：温度高了吹风降温，湿度高了排气除湿。
 * 两个需求各算一个目标转速，最后取较大的那个。 */
void App_Fan_Run(void) {
    for (;;) {
        SensorData_t data;
        uint16_t spd_temp, spd_humi, spd;

        osEventFlagsWait(EF_SensorData, EVENT_FAN_UPDATE, osFlagsWaitAll, osWaitForever);

        if (osMutexAcquire(Mutex_SensorData, 0U) == osOK) {
            data = SensorData;
            osMutexRelease(Mutex_SensorData);

            /* ---- 湿度闭环：超过上限就排气除湿 ---- */
            spd_humi = (data.Humidity > data.Humidity_Alarm[1])
                       ? FAN_SPEED_MID : FAN_SPEED_OFF;

            /* ---- 温度闭环：超出上限越多转越快 ---- */
            if (data.Temperature > data.Temperature_Alarm[1] &&
                data.Temperature < data.Temperature_Alarm[1] + 3) {
                spd_temp = FAN_SPEED_LOW;
            } else if (data.Temperature >= data.Temperature_Alarm[1] + 3 &&
                       data.Temperature < data.Temperature_Alarm[1] + 6) {
                spd_temp = FAN_SPEED_MID;
            } else if (data.Temperature >= data.Temperature_Alarm[1] + 6) {
                spd_temp = FAN_SPEED_HIGH;
            } else {
                spd_temp = FAN_SPEED_OFF;
            }

            /* 取两者中更大的转速 */
            spd = (spd_temp > spd_humi) ? spd_temp : spd_humi;
            Motor_SetSpeed(spd);
        }
    }
}
