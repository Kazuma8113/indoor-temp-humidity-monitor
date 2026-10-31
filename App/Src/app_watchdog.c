#include "app_watchdog.h"
#include "bsp_iwdg.h"      /* 本层第 2 处 Bsp 依赖（另见 app_tasks.c 的 bsp_tim.h） */
#include "cmsis_os.h"      /* osDelay */

/* ==================== 喂狗节奏 ====================
 * IWDG 由 MX_IWDG_Init() 启动：LSI/4、Reload 4095、12 位递减计数器。
 *   名义超时 = 4096 / (32000/4) = 4096 / 8000 ≈ 512 ms
 *   LSI 实测范围 17~47kHz，最坏（47kHz）= 4096 / 11750 ≈ 348 ms
 * 取 100ms 喂一次，对最坏情况仍有 3.5 倍余量。 */
#define WATCHDOG_FEED_MS   100U

/* ==================== 为什么是"最低优先级" ====================
 * 看门狗要发现的是"CPU 被某个任务长期霸占"，所以喂狗任务必须是抢不到 CPU 的那一个：
 *   - 若它优先级较高，一个死循环的低优先级任务卡住时，喂狗任务照样按时跑、照样喂，
 *     看门狗永远不会动作 —— 保护就失效了；
 *   - 放在最低优先级，"只要还有谁在霸占 CPU，它就轮不到" 这条才成立。
 * 具体取值 osPriorityIdle(1)：比 LCD_Task 的 osPriorityLow(8) 还低。
 *    不能只图"看起来低"就用 osPriorityLow —— LCD 任务已经占着这一档，
 *      同级之间有 1ms 时间片轮转，LCD 死循环时喂狗任务照样能被调度，保护就漏了。
 *
 * 它保护得到：调度器停摆、某任务死循环/长期不阻塞、节拍中断挂死。
 * 它保护不到：某个任务自己卡在一个永远不被释放的信号量上（那是合法阻塞，
 *   系统整体仍算"活着"）。那种情况要靠各任务的心跳计数，本任务没实现。
 *
 *  上电初始化阶段本任务抢不到 CPU（Init 任务优先级 High 且可能忙等），
 *    那段由 App_Init_Run() 里显式调 Bsp_Iwdg_Feed() 顶着，别删。 */
void App_Watchdog_Run(void) {
    for (;;) {
        Bsp_Iwdg_Feed();
        osDelay(WATCHDOG_FEED_MS);
    }
}
