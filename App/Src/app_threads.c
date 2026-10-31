#include "app_threads.h"

#include "app_tasks.h"     /* App_Init_Run / App_Led_Run / App_Dht11_Run / App_Bluetooth_Run */
#include "app_control.h"   /* App_Humidifier_Run / App_Warm_Run / App_Fan_Run */
#include "app_lcd.h"       /* App_Lcd_Run */
#include "app_watchdog.h"  /* App_Watchdog_Run */

/* ============================================================================
 * CubeMX 线程入口的唯一实现处。
 * ========================================================================== */

void Init_StartTask(void *argument) {
    (void) argument;
    App_Init_Run();          /* 内部结束时会 osThreadExit() */
}

void LED_StartTask(void *argument) {
    (void) argument;
    App_Led_Run();           
}

void LCD_StartTask(void *argument) {
    (void) argument;
    App_Lcd_Run();           
}

void DHT11_StartTask(void *argument) {
    (void) argument;
    App_Dht11_Run();         
}

void Bluetooth_StartTask(void *argument) {
    (void) argument;
    App_Bluetooth_Run();     
}

void Humidifier_StartTask(void *argument) {
    (void) argument;
    App_Humidifier_Run();    
}

void Warm_StartTask(void *argument) {
    (void) argument;
    App_Warm_Run();          
}

void Fan_StartTask(void *argument) {
    (void) argument;
    App_Fan_Run();          
}

/* 第 9 个任务：.ioc 里没有，由 freertos.c 的 USER CODE 段手工 osThreadNew()。
 * 之所以从 CubeMX 任务表里拿掉，是为了避免动 .ioc 重新生成（会覆盖 USER CODE、
 * 把 SPI3 挪回 PC10/PC12、重复 system_stm32f4xx.c）。 */
void Watchdog_StartTask(void *argument) {
    (void) argument;
    App_Watchdog_Run();      
}
