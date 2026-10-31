#include "app_tasks.h"
#include "app_data.h"

#include "Led.h"          /* LED_Flip / LED_Off（器件动作，引脚已内置） */
#include "app_led.h"      /* LED 业务状态机（心跳 / 快闪 / 常亮） */
#include "DHT11.h"
#include "TB6612.h"
#include "Humidifier.h"   /* Humidifier_Init()：上电先关 */
#include "Warm.h"         /* Warm_Init()：上电先关 */
#include "lcd_spi_154.h"
#include "ble_data_parse.h"
#include "app_tick.h"     /* Sem2_*（任务节拍信号量，本层） */
#include "bsp_tim.h"      /* Bsp_Tim_Start() */
#include "bsp_iwdg.h"     /* Bsp_Iwdg_Feed()：初始化期间顶住看门狗（共 2 处 Bsp 依赖） */

#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

/* ==================== 上电一次性初始化 ==================== */
void App_Init_Run(void) {
    /* IWDG 在 main() 里就已经启动（约 512ms 超时，启动后关不掉），而喂狗任务是
     * 全工程最低优先级，初始化阶段抢不到 CPU。所以这一段必须手动喂几次顶过去。 */
    Bsp_Iwdg_Feed();

    /* LCD 没有显存、是直接写屏；字符/文字函数会同时填前景和背景像素，
     * 所以按固定坐标、固定宽度重绘即可，既不会闪烁也不会留残影。 */
    SPI_LCD_Init();
    Bsp_Iwdg_Feed();       /* 屏初始化里有一串 ms 级延时，喂一次再往下走 */
    DHT11_Init();          /* 内部顺带初始化 DWT（µs 延时的底座） */
    Motor_Init();
    Humidifier_Init();     /* 上电先关：不依赖 CubeMX 配的初始电平 */
    Warm_Init();           /* 同上，加热片低有效，必须先明确拉高 */
    Bluetooth_Init();      /* 串口 + DMA 接收 + IDLE 中断，先把外设备好 */
    Bsp_Iwdg_Feed();       /* 外设都就绪，之后交给看门狗任务按 100ms 喂 */
    Bsp_Tim_Start();       /* 最后开节拍：外设都就绪了，任务才不会跑起来撞空 */

    osThreadExit();   /* 一次性任务，做完就退出 */
}

/* ==================== LED 心跳 / 快闪 ==================== */
void App_Led_Run(void) {
    for (;;) {
        uint8_t LED_Status;
        osSemaphoreAcquire(Sem2_LED, osWaitForever);

        /* 先加锁把状态取到局部变量，随即解锁（App_Led_GetState 内部自带锁） */
        LED_Status = App_Led_GetState();

        switch (LED_Status) {
            case LED_STATE_DHT11_FAIL:
                LED_Flip(200);   /* 0.2s 快闪：DHT11 没起来 */
                break;

            case LED_STATE_ON:
                LED_Flip(500);   /* 0.5s 心跳：正常运行 */
                break;

            case LED_STATE_OFF:
            default:
                LED_Off();
                break;
        }
    }
}

/* ==================== 温湿度采集 ====================
 * DHT11 是唯一的数据源，每轮读取成功后就一次性通知所有消费者。 */
void App_Dht11_Run(void) {
    uint8_t temperature, humidity;

    for (;;) {
        osSemaphoreAcquire(Sem2_DHT11, osWaitForever);

        /* DHT11 读取很慢（内部有 ms 级延时），先读到局部变量，再短时间加锁写入共享区 */
        if (DHT11_Read_Data(&temperature, &humidity) == 0) {
            if (osMutexAcquire(Mutex_SensorData, 0U) == osOK) {
                SensorData.Temperature = temperature;
                SensorData.Humidity = humidity;
                osMutexRelease(Mutex_SensorData);

                /* 写完即整组就绪，通知所有消费者 */
                osEventFlagsSet(EF_SensorData, EVENT_DATA_COMPLETE);
            }
        }
    }
}

/* ==================== 蓝牙：收指令 / 回 ACK / 上报 ==================== */
void App_Bluetooth_Run(void) {
    for (;;) {
        uint16_t len;
        uint32_t Flag;
        char text[64] = {0};
        SensorData_t data;    /* 上报用的本地副本，不直接碰共享区 */
        Ble_Threshold_t th;   /* 协议层解析出来的 4 个阈值字段 */
        /* 千万不能提到 for 外面，因为不是每次都能顺利获取互斥锁 */
        Ble_Status_t status = BT_STATUS_BUSY;
        Ble_LedCmd_t led_cmd = BT_LED_CMD_NONE;

        osSemaphoreAcquire(Sem2_Bluetooth, osWaitForever);

        /* 锁外解析：取帧、握手判断、sscanf，结果落进本地 th。全程不碰 SensorData / LED 状态，所以这里不需要持锁。 */
        memset(&th, 0, sizeof(th));
        status = Bluetooth_ReceiveHandler(&th, &led_cmd);

        if (status == BT_DHT_STATUS_SUCCESS) {
            if (osMutexAcquire(Mutex_SensorData, 0U) == osOK) {
                SensorData.Temperature_Alarm[0] = th.Temp_Min;
                SensorData.Temperature_Alarm[1] = th.Temp_Max;
                SensorData.Humidity_Alarm[0]    = th.Hum_Min;
                SensorData.Humidity_Alarm[1]    = th.Hum_Max;
                osMutexRelease(Mutex_SensorData);
            } else {
                status = BT_STATUS_BUSY;  // 抢不到锁不能回 [SET OK!]，降级成 BUSY 让手机下一轮重发。
            }
        }

        /* App_Led_SetState 内部自带锁 */
        if (led_cmd == BT_LED_CMD_ON) {
            App_Led_SetState(LED_STATE_ON);
        } else if (led_cmd == BT_LED_CMD_OFF) {
            App_Led_SetState(LED_STATE_OFF);
        }

        /* 回话放在锁外发，防止持锁阻塞会拖住 DHT11 写共享数据。 */
        Bluetooth_ReplyByStatus(status);

        Flag = osEventFlagsWait(EF_SensorData, EVENT_BLUE_UPDATE, osFlagsWaitAll, 0U);
        if ((Flag & 0x80000000U) == 0U && (Flag & EVENT_BLUE_UPDATE)) {
            if (osMutexAcquire(Mutex_SensorData, 0U) == osOK) {
                data = SensorData;      /* 加锁拷贝快照，立刻解锁 */
                osMutexRelease(Mutex_SensorData);

                len = sprintf(text, "温度:%u%% 湿度:%u%%\n", data.Temperature, data.Humidity);
                /* 没发出去（线路一直忙 / DMA 启动失败）就把事件位重新置起来，
                 * 让下一拍 200ms 补报一次，而不是静默丢一帧。 */
                if (Bluetooth_SendHandler((const uint8_t *) text, len) == 0U) {
                    osEventFlagsSet(EF_SensorData, EVENT_BLUE_UPDATE);
                }
            }
        }
    }
}
