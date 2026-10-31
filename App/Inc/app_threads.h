#ifndef APP_THREADS_H
#define APP_THREADS_H

/* ==================== CubeMX 线程入口 ====================
 *
 * 这 9 个函数名是 CubeMX (.ioc 的 FREERTOS.Tasks01) 里的 "Entry Function"，
 * MX_FREERTOS_Init() 用 osThreadNew() 把它们注册成 FreeRTOS 线程。
 * 最后一个 Watchdog_StartTask 的优先级是 1（osPriorityIdle），全工程最低。

 * 每个函数体只有一行：转调 App 层的 *_Run()。真正的循环逻辑在
 * app_tasks.c / app_control.c / app_lcd.c 里。
 * ------------------------------------------------------------------ */

void Init_StartTask(void *argument);        /* → App_Init_Run()       一次性初始化，内部 osThreadExit() */
void LED_StartTask(void *argument);         /* → App_Led_Run()        LED 心跳 / 快闪 */
void LCD_StartTask(void *argument);         /* → App_Lcd_Run()        LCD 刷新 */
void DHT11_StartTask(void *argument);       /* → App_Dht11_Run()      温湿度采集 */
void Bluetooth_StartTask(void *argument);   /* → App_Bluetooth_Run()  蓝牙收发 */
void Humidifier_StartTask(void *argument);  /* → App_Humidifier_Run() 湿度闭环 */
void Warm_StartTask(void *argument);        /* → App_Warm_Run()       温度闭环 */
void Fan_StartTask(void *argument);         /* → App_Fan_Run()        风扇双闭环 */
void Watchdog_StartTask(void *argument);    /* → App_Watchdog_Run()   喂狗 */

#endif /* APP_THREADS_H */
