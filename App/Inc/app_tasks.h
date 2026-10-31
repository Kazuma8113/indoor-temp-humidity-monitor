#ifndef APP_TASKS_H
#define APP_TASKS_H


void App_Init_Run(void);        /* 上电一次性初始化，结束时 osThreadExit() */
void App_Led_Run(void);         /* LED 心跳 / 快闪 */
void App_Dht11_Run(void);       /* 采温湿度并整组广播事件 */
void App_Bluetooth_Run(void);   /* 收指令、回 ACK、上报温湿度 */

#endif /* APP_TASKS_H */
