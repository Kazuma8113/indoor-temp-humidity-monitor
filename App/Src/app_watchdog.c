#include "app_watchdog.h"

#include "bsp_iwdg.h"      /* 本层第 2 处 Bsp 依赖（另见 app_tasks.c 的 bsp_tim.h） */
#include "app_diag.h"      /* App_Diag_Scan()：超期判定；APP_DIAG_REASON_*：复位原因 */
#include "lcd_spi_154.h"   /* 故障停机界面：SPI_LCD_Init / LCD_DisplayString，顺带引入 HAL_Delay */
#include "cmsis_os.h"      /* osDelay */

/* ==================== 喂狗节奏 ====================
 * IWDG 由 MX_IWDG_Init() 启动：LSI/4、Reload 4095、12 位递减计数器。
 * 名义超时 = 4096 / (32000/4) = 4096 / 8000 ≈ 512 ms
 * LSI 实测范围 17~47kHz，最坏（47kHz）= 4096 / 11750 ≈ 348 ms
 * 取 100ms 喂一次，对最坏情况仍有 3.5 倍余量。 */
#define WATCHDOG_FEED_MS   100U

/* ==================== 喂狗任务主体 ====================
 * 正常时每 100ms 喂一次，同时顺带做一次超期判定。
 * 内部死循环，永不返回。 */
void App_Watchdog_Run(void) {
    for (;;) {
        App_Diag_Kick(APP_DIAG_TASK_WATCHDOG);

        if (App_Diag_Scan() == APP_DIAG_RET_FATAL) {
            /* 确认有任务卡死：停止喂狗，让 IWDG 用独立 LSI 时钟复位。
             * 这里不画屏 —— 距复位只剩 348ms，而且卡住的可能正是显示任务；
             * 故障信息统一交给下次启动的 FaultScreen()，在调度器起来之前显示。 */
            for (;;) {
            }
        }

        Bsp_Iwdg_Feed();
        osDelay(WATCHDOG_FEED_MS);
    }
}

/* ==================== 取本次复位原因 ====================
 * 返回值对应 app_diag.h 的 APP_DIAG_REASON_*。
 * 所以只能在启动阶段调用一次；再调会拿到 APP_DIAG_REASON_NONE。 */
uint32_t App_Watchdog_ResetCause(void) {
    uint8_t is_wdg = Bsp_Iwdg_IsResetFromWatchdog();
    Bsp_Iwdg_ClearResetFlags();

    return (is_wdg != 0U) ? APP_DIAG_REASON_IWDG : APP_DIAG_REASON_NONE;
}

/* ==================== 故障停机界面 ====================
 * 上一轮被看门狗咬、且记下了元凶时，停在画面上报错，等人工按复位键。
 * 必须在调度器启动之前调用（main 里、osKernelInitialize() 之前）*/
void App_Watchdog_FaultScreen(uint32_t cause) {
#if (APP_DIAG_SHOW_FAULT == 0U)
    /* 开关关掉：不停机报错，直接放行让系统正常启动。
     * 这样故障时会被 IWDG 反复复位重试，适合无人值守。 */
    (void) cause;
    return;
#else
    /* 普通复位、或者狗咬时监控任务自己都没跑起来（last_blame 为无），一律放行 */
    if (cause != APP_DIAG_REASON_IWDG ||
        g_diag.last_blame == APP_DIAG_NO_BLAME) {
        return;
    }

    /* SPI_LCD_Init() 里有复位序列和 ms 级延时，前后各喂一次撑过去 */
    Bsp_Iwdg_Feed();
    SPI_LCD_Init();
    Bsp_Iwdg_Feed();

    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LCD_WHITE);

    /* 三行：谁死了、沉默多久、怎么恢复 */
    LCD_DisplayString(Lie(1), 70U, "Error Task: ");
    LCD_DisplayString(Lie(1) + 12U * 12U, 70U,
                      App_Diag_GetTaskName(g_diag.last_blame));

    LCD_DisplayString(Lie(1), 110U, "Age: ");
    LCD_DisplayNumber(Lie(1) + 5U * 12U, 110U, (int32_t) g_diag.last_age, 4U);
    LCD_DisplayString(Lie(1) + 9U * 12U, 110U, " ms");

    LCD_DisplayString(Lie(1), 150U, "Reset to retry");

    /* 停住，持续喂狗：系统不再复位，等人工干预 */
    for (;;) {
        Bsp_Iwdg_Feed();
        HAL_Delay(WATCHDOG_FEED_MS);
    }
#endif /* APP_DIAG_SHOW_FAULT */
}
