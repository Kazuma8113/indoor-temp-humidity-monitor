#ifndef APP_WATCHDOG_H
#define APP_WATCHDOG_H

#include <stdint.h>

/* 函数说明见 app_watchdog.c */
void     App_Watchdog_Run(void);
uint32_t App_Watchdog_ResetCause(void);
void     App_Watchdog_FaultScreen(uint32_t cause);

#endif /* APP_WATCHDOG_H */
