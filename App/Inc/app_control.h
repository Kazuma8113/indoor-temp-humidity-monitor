#ifndef APP_CONTROL_H
#define APP_CONTROL_H


void App_Humidifier_Run(void);   /* 湿度闭环：低于下限开加湿，到上限关 */
void App_Warm_Run(void);         /* 温度闭环：低于下限加热，到"下限+回差"关 */
void App_Fan_Run(void);          /* 温度 + 湿度双闭环取较大档位 */

#endif /* APP_CONTROL_H */
