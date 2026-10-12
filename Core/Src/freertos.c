/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DWT.h"
#include "Led.h"
#include "lcd_spi_154.h"   /* 板载 SPI 液晶屏（ST7789, 240x240）驱动 */
#include "DHT11.h"
#include "TB6612.h"
#include "usart.h"
#include "tim.h"
#include "Warm.h"
#include "Humidifier.h"
#include "Bluetooth.h"

#include <stdio.h>
#include <string.h>


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* 每位只对应一个消费者，避免多个任务等待同一位时被 osEventFlagsWait 抢先清零。 */
#define EVENT_LCD_UPDATE    (1U << 0)   /* LCD 刷新 */
#define EVENT_BLUE_UPDATE   (1U << 1)   /* 蓝牙上报 */
#define EVENT_HUMI_UPDATE   (1U << 2)   /* 加湿控制 */
#define EVENT_WARM_UPDATE   (1U << 3)   /* 加热控制 */
#define EVENT_FAN_UPDATE    (1U << 4)   /* 风扇控制 */

/* DHT11 是唯一的数据源（MQ2 已移除），每轮读取成功后就一次性通知所有消费者。 */
#define EVENT_DATA_COMPLETE    (EVENT_LCD_UPDATE | EVENT_BLUE_UPDATE | \
                                EVENT_HUMI_UPDATE | EVENT_WARM_UPDATE | \
                                EVENT_FAN_UPDATE)

/* ===== LCD 上要显示的中文 ===============================================
 * 字库 (Hardware/LCD/lcd_fonts.c) 与驱动 (lcd_spi_154.c) 已统一为 UTF-8：
 * 汉字索引按 UTF-8 三字节存放，驱动按三字节比对，本文件也是 UTF-8，
 * 所以下面可以直接写中文。
 * 注意：只有已收录进 Chinese_2424 字库的字才能显示；要加新字就在 lcd_fonts.c
 *       的 Chinese_2424 表里补「一行点阵 + 一行索引」，索引行写该汉字即可。
 */
#define LCD_TXT_TITLE   "室内温湿度监测"
#define LCD_TXT_TEMP    "温度:"
#define LCD_TXT_HUMI    "湿度:"
#define LCD_TXT_LOW     "下限"
#define LCD_TXT_HIGH    "上限"
/* ======================================================================= */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */



/* 全局唯一的共享数据实例（零初始化，避免首次读取到随机值） */
SensorData_t SensorData = {
        .Temperature = 0,
        .Temperature_Alarm = {20, 25},
        .Humidity = 0,
        .Humidity_Alarm = {50, 70}
};

/* USER CODE END Variables */
/* Definitions for Init_Task */
osThreadId_t Init_TaskHandle;
const osThreadAttr_t Init_Task_attributes = {
  .name = "Init_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh1,
};
/* Definitions for LED_Task */
osThreadId_t LED_TaskHandle;
const osThreadAttr_t LED_Task_attributes = {
  .name = "LED_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LCD_Task */
osThreadId_t LCD_TaskHandle;
const osThreadAttr_t LCD_Task_attributes = {
  .name = "LCD_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for DHT11_Task */
osThreadId_t DHT11_TaskHandle;
const osThreadAttr_t DHT11_Task_attributes = {
  .name = "DHT11_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for Bluetooth_Task */
osThreadId_t Bluetooth_TaskHandle;
const osThreadAttr_t Bluetooth_Task_attributes = {
  .name = "Bluetooth_Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for Humidifier_Task */
osThreadId_t Humidifier_TaskHandle;
const osThreadAttr_t Humidifier_Task_attributes = {
  .name = "Humidifier_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Warm_Task */
osThreadId_t Warm_TaskHandle;
const osThreadAttr_t Warm_Task_attributes = {
  .name = "Warm_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Fan_Task */
osThreadId_t Fan_TaskHandle;
const osThreadAttr_t Fan_Task_attributes = {
  .name = "Fan_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* 互斥锁：保护共享数据 SensorData，防止多个任务同时读写 */
osMutexId_t Mutex_SensorData;
osMutexAttr_t attr_SensorData = {
        .attr_bits = osMutexRecursive | osMutexPrioInherit
};

/* 二值信号量：用于控制任务资源，初始值为 1 表示资源可用 */
osSemaphoreId_t Sem2_LED;
osSemaphoreId_t Sem2_LCD;
osSemaphoreId_t Sem2_DHT11;
osSemaphoreId_t Sem2_Bluetooth;

/* 事件标志组：通知消费者"数据已就绪"*/
osEventFlagsId_t EF_SensorData;


/* USER CODE END FunctionPrototypes */

void Init_StartTask(void *argument);
void LED_StartTask(void *argument);
void LCD_StartTask(void *argument);
void DHT11_StartTask(void *argument);
void Bluetooth_StartTask(void *argument);
void Humidifier_StartTask(void *argument);
void Warm_StartTask(void *argument);
void Fan_StartTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    Mutex_SensorData = osMutexNew(&attr_SensorData);

    /* LED_State 的互斥锁在 Led.c 内部管理，这里只负责创建 */
    LED_State_Init();

  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* 全部初始可用 */
    Sem2_LED = osSemaphoreNew(1, 1, NULL);
    Sem2_LCD = osSemaphoreNew(1, 1, NULL);
    Sem2_DHT11 = osSemaphoreNew(1, 1, NULL);
    Sem2_Bluetooth = osSemaphoreNew(1, 1, NULL);

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* 已改为"共享数据 SensorData + Mutex_SensorData + EF_SensorData"，
     * 不再使用消息队列 */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Init_Task */
  Init_TaskHandle = osThreadNew(Init_StartTask, NULL, &Init_Task_attributes);

  /* creation of LED_Task */
  LED_TaskHandle = osThreadNew(LED_StartTask, NULL, &LED_Task_attributes);

  /* creation of LCD_Task */
  LCD_TaskHandle = osThreadNew(LCD_StartTask, NULL, &LCD_Task_attributes);

  /* creation of DHT11_Task */
  DHT11_TaskHandle = osThreadNew(DHT11_StartTask, NULL, &DHT11_Task_attributes);

  /* creation of Bluetooth_Task */
  Bluetooth_TaskHandle = osThreadNew(Bluetooth_StartTask, NULL, &Bluetooth_Task_attributes);

  /* creation of Humidifier_Task */
  Humidifier_TaskHandle = osThreadNew(Humidifier_StartTask, NULL, &Humidifier_Task_attributes);

  /* creation of Warm_Task */
  Warm_TaskHandle = osThreadNew(Warm_StartTask, NULL, &Warm_Task_attributes);

  /* creation of Fan_Task */
  Fan_TaskHandle = osThreadNew(Fan_StartTask, NULL, &Fan_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
    EF_SensorData = osEventFlagsNew(NULL);
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Init_StartTask */
/**
  * @brief  Function implementing the Init_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Init_StartTask */
void Init_StartTask(void *argument)
{
  /* USER CODE BEGIN Init_StartTask */
    DWT_Init();
    /* LCD 没有显存、是直接写屏；字符/文字函数会同时填前景和背景像素，
     * 所以按固定坐标、固定宽度重绘即可，既不会闪烁也不会留残影。 */
    SPI_LCD_Init();
    // 判断DHT11是否连接成功
    while (1) { if (DHT11_Init() == 0) break; }
    // 等待DHT11初始化完成
    while (1) {
        DHT11_Read_Data(&SensorData.Temperature, &SensorData.Humidity);
        if (SensorData.Temperature != 0 && SensorData.Humidity != 0) break;
    }
    Motor_Init();
    Usart1_Init();
    HAL_TIM_Base_Start_IT(&htim1);

    osThreadExit();
  /* USER CODE END Init_StartTask */
}

/* USER CODE BEGIN Header_LED_StartTask */
/**
  * @brief  Function implementing the LED_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_LED_StartTask */
void LED_StartTask(void *argument)
{
  /* USER CODE BEGIN LED_StartTask */
    /* Infinite loop */
    for (;;) {
        uint8_t led_on;

        osSemaphoreAcquire(Sem2_LED, osWaitForever);

        /* 先加锁把状态取到局部变量，随即解锁。
         * ⚠️ 绝不能把锁带进 LED_Flip —— 它内部的 osDelayUntil 会阻塞 500ms，
         *    那样蓝牙任务发 LED_ON/LED_OFF 就得跟着等半秒。 */
        led_on = LED_State_Get();

        // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        if (led_on == 1) {
            LED_Flip(LED_PC13, 500);
        } else {
            LED_Off(LED_PC13);
        }

    }
  /* USER CODE END LED_StartTask */
}

/* USER CODE BEGIN Header_LCD_StartTask */
/**
* @brief Function implementing the LCD_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LCD_StartTask */
void LCD_StartTask(void *argument)
{
  /* USER CODE BEGIN LCD_StartTask */
    /* Infinite loop */
    for (;;) {
        SensorData_t data;
        uint32_t Flag;

        /* 刷新节奏由定时信号量控制（100 ms） */
        osSemaphoreAcquire(Sem2_LCD, osWaitForever);
        /* 等待"数据已更新"通知 */
        Flag = osEventFlagsWait(EF_SensorData, EVENT_LCD_UPDATE, osFlagsWaitAll, 0);

        if (Flag & EVENT_LCD_UPDATE) {
            /* 加锁拷贝快照 */
            if (osMutexAcquire(Mutex_SensorData, 0) == osOK) {
                data = SensorData;
                osMutexRelease(Mutex_SensorData);

                /* ===== LCD 版面（240x240，行高 24，按 24px 网格排版）==========
                 * 文本用 24x24 汉字库（配 24x12 ASCII）。LCD 无显存、是直写屏，
                 * 字符函数会同时填前景和背景像素，所以按固定坐标重绘既不会闪、也不会留残影。 */
                LCD_SetTextFont(&CH_Font24);

                /* ---- 标题：7 个汉字共 168px，居中 → x = (240-168)/2 = 36 = Lie(3) ---- */
                LCD_DisplayText(Lie(3), Hang(1), LCD_TXT_TITLE);

                /* ---- 分隔线：用灰色，比文字暗，起分区作用又不抢视线 ---- */
                LCD_SetColor(0x808080);
                LCD_DrawLine_H(Lie(1), Hang(2) + 10, 216);   /* 标题下 */
                LCD_DrawLine_H(Lie(1), Hang(5) + 10, 216);   /* 温度区 / 湿度区 之间 */
                LCD_DrawLine_H(Lie(1), Hang(8) + 10, 216);   /* 底部 */
                LCD_SetColor(LCD_WHITE);                     /* 恢复画笔色，后面的文字才是白的 */

                /* ---- 温度：标签 Lie(2) / 数值 Lie(8) / 单位 Lie(11) 三列对齐
                 *      （单位列与下面"上限"标签列同列 = Lie(11)，视觉更整齐；
                 *        数值列取 Lie(8) 是为了湿度可能出现的 3 位数（100%RH）留出余量） ---- */
                LCD_DisplayText(Lie(2), Hang(3), LCD_TXT_TEMP);
                LCD_DisplayNumber(Lie(8), Hang(3), data.Temperature, 2);
                LCD_DisplayText(Lie(11), Hang(3), "℃");
                LCD_DisplayText(Lie(2), Hang(4), LCD_TXT_LOW);
                LCD_DisplayNumber(Lie(7), Hang(4), data.Temperature_Alarm[0], 2);
                LCD_DisplayText(Lie(11), Hang(4), LCD_TXT_HIGH);
                LCD_DisplayNumber(Lie(16), Hang(4), data.Temperature_Alarm[1], 2);

                /* ---- 湿度：与温度同样的三列对齐 ---- */
                LCD_DisplayText(Lie(2), Hang(6), LCD_TXT_HUMI);
                LCD_DisplayNumber(Lie(8), Hang(6), data.Humidity, 2);
                LCD_DisplayText(Lie(11), Hang(6), "%RH");
                LCD_DisplayText(Lie(2), Hang(7), LCD_TXT_LOW);
                LCD_DisplayNumber(Lie(7), Hang(7), data.Humidity_Alarm[0], 2);
                LCD_DisplayText(Lie(11), Hang(7), LCD_TXT_HIGH);
                LCD_DisplayNumber(Lie(16), Hang(7), data.Humidity_Alarm[1], 2);
            }

        }
    }
  /* USER CODE END LCD_StartTask */
}

/* USER CODE BEGIN Header_DHT11_StartTask */
/**
* @brief Function implementing the DHT11_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_DHT11_StartTask */
void DHT11_StartTask(void *argument)
{
  /* USER CODE BEGIN DHT11_StartTask */
    /* Infinite loop */
    uint8_t temperature, humidity;
    for (;;) {
        osSemaphoreAcquire(Sem2_DHT11, osWaitForever);

        /* DHT11 读取很慢（内部有 ms 级延时），先读到局部变量，再短时间加锁写入共享区 */
        if (DHT11_Read_Data(&temperature, &humidity) == 0) {
            if (osMutexAcquire(Mutex_SensorData, 0) == osOK) {
                SensorData.Temperature = temperature;
                SensorData.Humidity = humidity;
                osMutexRelease(Mutex_SensorData);

                /* DHT11 是唯一的数据源，写完即整组就绪，通知所有消费者 */
                osEventFlagsSet(EF_SensorData, EVENT_DATA_COMPLETE);
            }
        }
    }
  /* USER CODE END DHT11_StartTask */
}

/* USER CODE BEGIN Header_Bluetooth_StartTask */
/**
* @brief Function implementing the Bluetooth_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Bluetooth_StartTask */
void Bluetooth_StartTask(void *argument)
{
  /* USER CODE BEGIN Bluetooth_StartTask */
    /* Infinite loop */
    for (;;) {
        uint16_t len;
        uint32_t Flag;
        char text[50] = {0};
        SensorData_t data;       /* 本地副本：读写都走它，不直接碰共享区 */
        Bluetooth_CmdKind_t kind = BT_CMD_NONE;
        Bluetooth_Status_t status = BT_STATUS_BUSY; // 千万不能提到 for 外面，因为不是每次都能顺利获取互斥锁

        osSemaphoreAcquire(Sem2_Bluetooth, osWaitForever);

        /* 锁外解析：读串口缓冲、握手判断、sscanf，结果全部落进本地 data。
         * 全程不碰 SensorData / LED_State，所以这里不需要持锁。 */
        memset(&data, 0, sizeof(data));
        status = Bluetooth_ReceiveHandler(&data, &kind);

        /* 锁内只做"写回"这一件事：4 个阈值字段整体写入，
         * 读者（LCD / 加湿 / 加热 / 风扇）看不到"新下限配旧上限"的半更新组合。
         * 抢不到锁不能回 [SET OK!]，降级成 BUSY 让手机下一轮重发。 */
        if (status == BT_STATUS_SUCCESS && kind == BT_CMD_THRESHOLD) {
            if (osMutexAcquire(Mutex_SensorData, 0) == osOK) {
                SensorData.Temperature_Alarm[0] = data.Temperature_Alarm[0];
                SensorData.Temperature_Alarm[1] = data.Temperature_Alarm[1];
                SensorData.Humidity_Alarm[0] = data.Humidity_Alarm[0];
                SensorData.Humidity_Alarm[1] = data.Humidity_Alarm[1];
                osMutexRelease(Mutex_SensorData);
            } else {
                status = BT_STATUS_BUSY;
            }
        }
        /* kind == BT_CMD_LED 时什么都不用做：LED_State 已在 Bluetooth_GetData 里加锁改完 */

        /* 回话放在锁外发：SendHandler 发送忙时会 osDelay 等最长 100ms，
         * 持锁阻塞会拖住 DHT11 写共享数据。
         * 该函数里已经包含了 Bluetooth_ReceiveHandler 发送函数*/
        Bluetooth_ReplyByStatus(status);

        /* 上报 DHT11 温湿度信息：锁内只拷快照，sprintf 与发送都在锁外 */
        Flag = osEventFlagsWait(EF_SensorData, EVENT_BLUE_UPDATE, osFlagsWaitAll, 0);
        if (Flag & EVENT_BLUE_UPDATE) {
            if (osMutexAcquire(Mutex_SensorData, 0) == osOK) {
                data = SensorData;      /* 加锁拷贝快照，立刻解锁 */
                osMutexRelease(Mutex_SensorData);

                len = sprintf(text, "温度:%u%% 湿度:%u%%\n",
                              data.Temperature, data.Humidity);
                Bluetooth_SendHandler((uint8_t *) text, len);
            }
        }
    }
  /* USER CODE END Bluetooth_StartTask */
}

/* USER CODE BEGIN Header_Humidifier_StartTask */
/**
* @brief Function implementing the Humidifier_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Humidifier_StartTask */
void Humidifier_StartTask(void *argument)
{
  /* USER CODE BEGIN Humidifier_StartTask */
    /* Infinite loop */
    //根据湿度，进行加湿
    for (;;) {
        SensorData_t data;
        osEventFlagsWait(EF_SensorData, EVENT_HUMI_UPDATE, osFlagsWaitAll, osWaitForever);

        if (osMutexAcquire(Mutex_SensorData, 0) == osOK) {
            data = SensorData;
            osMutexRelease(Mutex_SensorData);

            if (data.Humidity < data.Humidity_Alarm[0]) {
                Humidifier_Set(GPIO_PIN_SET);
            } else {
                Humidifier_Set(GPIO_PIN_RESET);
            }

        }
    }
  /* USER CODE END Humidifier_StartTask */
}

/* USER CODE BEGIN Header_Warm_StartTask */
/**
* @brief Function implementing the Warm_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Warm_StartTask */
void Warm_StartTask(void *argument)
{
  /* USER CODE BEGIN Warm_StartTask */
    /* Infinite loop */
    //根据温度是否太低，进行升温
    for (;;) {
        SensorData_t data;
        osEventFlagsWait(EF_SensorData, EVENT_WARM_UPDATE, osFlagsWaitAll, osWaitForever);

        if (osMutexAcquire(Mutex_SensorData, 0) == osOK) {
            data = SensorData;
            osMutexRelease(Mutex_SensorData);

            if (data.Temperature < data.Temperature_Alarm[0]) {
                Warm_Set(GPIO_PIN_RESET);
            } else {
                Warm_Set(GPIO_PIN_SET);
            }
        }


    }
  /* USER CODE END Warm_StartTask */
}

/* USER CODE BEGIN Header_Fan_StartTask */
/**
* @brief Function implementing the Fan_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Fan_StartTask */
void Fan_StartTask(void *argument)
{
  /* USER CODE BEGIN Fan_StartTask */
    /* Infinite loop */
    // 根据温度，进行降温
    for (;;) {
        SensorData_t data;

        osEventFlagsWait(EF_SensorData, EVENT_FAN_UPDATE, osFlagsWaitAll, osWaitForever);

        if (osMutexAcquire(Mutex_SensorData, 0) == osOK) {
            data = SensorData;
            osMutexRelease(Mutex_SensorData);



            if (data.Temperature > data.Temperature_Alarm[1] &&
                data.Temperature < data.Temperature_Alarm[1] + 3) {
                Motor_SetSpeed(60);
            } else if (data.Temperature >= data.Temperature_Alarm[1] + 3 &&
                       data.Temperature < data.Temperature_Alarm[1] + 6) {
                Motor_SetSpeed(80);
            } else if (data.Temperature >= data.Temperature_Alarm[1] + 6) {
                Motor_SetSpeed(100);
            } else {

                Motor_SetSpeed(0);
            }
        }
    }
  /* USER CODE END Fan_StartTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */


/* USER CODE END Application */

