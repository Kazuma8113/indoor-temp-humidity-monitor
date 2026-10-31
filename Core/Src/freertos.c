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
/* 本文件只做"创建"：RTOS 对象由 App_Data_Init() 建，线程体在 App/Src/app_threads.c */
#include "app_data.h"      /* SensorData / 互斥锁 / 事件标志 + App_Data_Init() */
#include "app_tick.h"      /* 任务节拍的 4 个二值信号量 Sem2_*（释放由 TIM1 中断做） */
#include "app_threads.h"   /* 线程入口原型：不依赖下面生成区的声明也能编译 */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* 共享数据、互斥锁、二值信号量、事件标志组、控制参数（回差 / 风扇档位 / LCD 文案）
 * 全部在 App 层：
 *     App/Src/app_data.c      SensorData + Mutex_SensorData + EF_SensorData
 *     App/Src/app_tick.c      Sem2_*（4 个节拍信号量）+ TIM1 分频
 *     App/Src/app_control.c   回差与风扇档位策略
 *     App/Src/app_lcd.c       LCD 版面与文案
 *     App/Src/app_threads.c   8 个线程入口（Init_StartTask ... Fan_StartTask）
 * 本文件只保留 CubeMX 生成的线程句柄与属性表。 */
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
  .priority = (osPriority_t) osPriorityNormal,
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
/* Definitions for Watchdog_Task */
osThreadId_t Watchdog_TaskHandle;
const osThreadAttr_t Watchdog_Task_attributes = {
  .name = "Watchdog_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityIdle,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

extern void Init_StartTask(void *argument);
extern void LED_StartTask(void *argument);
extern void LCD_StartTask(void *argument);
extern void DHT11_StartTask(void *argument);
extern void Bluetooth_StartTask(void *argument);
extern void Humidifier_StartTask(void *argument);
extern void Warm_StartTask(void *argument);
extern void Fan_StartTask(void *argument);
extern void Watchdog_StartTask(void *argument);

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
    /* 业务同步对象（互斥锁 / 事件标志）在 App 层创建，必须早于任务创建。 */
    App_Data_Init();
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* 任务节拍的 4 个二值信号量（定时 1ms、TIM1 中断按 100/100/1000/200 释放）。
     * 必须赶在任何任务创建之前建好，否则任务拿到的是空句柄。 */
    App_Sem_Init();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */

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

  /* creation of Watchdog_Task */
  Watchdog_TaskHandle = osThreadNew(Watchdog_StartTask, NULL, &Watchdog_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
    /* 看门狗任务已在 .ioc 里配好（Watchdog_Task，优先级 1 = osPriorityIdle），
     * 上面的属性表和创建语句都由 CubeMX 生成，重新生成代码时不会重复。
     * 本段只保留创建失败的兜底：堆不足时不能静默继续，否则 IWDG 512ms 后一直复位。 */
    if (Watchdog_TaskHandle == NULL) {
      Error_Handler();
    }
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
    /* EF_SensorData 已由 App_Data_Init() 创建 */
  /* USER CODE END RTOS_EVENTS */

}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

