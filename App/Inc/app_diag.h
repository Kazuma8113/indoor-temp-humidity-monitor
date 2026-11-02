#ifndef APP_DIAG_H
#define APP_DIAG_H

#include <stdint.h>

/* ============================================================================
 * 故障诊断保留区（.noinit）
 *
 * 目的：让 IWDG 复位后还能知道"上一轮是哪个任务挂的"。
 * 手段：把记录放进一块复位不清零的 RAM（.noinit 段，链接脚本钉死在
 *       0x10000000 的 CCM RAM），startup 的 LoopFillZerobss 只清
 *       _sbss~_ebss，不会碰它。
 *
 * 注意：这不是 Flash 存储。IWDG 复位是内核复位、不断电，RAM 内容本来就在，
 *       没有任何理由搬到 Flash 去（Flash 擦写慢、寿命只有 1 万次）。
 *       代价是掉电就丢——但掉电时任务全停，本来也没有"故障任务"可记。
 * ========================================================================== */

/* 本区有效标记 */
#define APP_DIAG_MAGIC      0x57445449U   /* 'WDTG' + 2 */

/* "没有元凶"的统一写法。0xFF 大于任何合法编号，不会和真实任务撞上。 */
#define APP_DIAG_NO_BLAME   0xFFU

/* ==================== 显示开关（编译期） ====================
 * 只控制"显示"，不影响检测和复位：
 *   App_Diag_Scan() 照常判定，超期照样停喂狗，只是屏上说不说而已。
 * 两个开关互相独立：
 *   关 SHOW_WARN  —— 运行中不在屏底显示超期任务，界面保持干净，适合演示
 *   关 SHOW_FAULT —— 复位后不停机报错，直接正常启动（无人值守 / 自动重试场景）
 * 改完必须重新编译烧录，运行时改不了。 */
#define APP_DIAG_SHOW_WARN      1U   /* 1 = 运行中显示 "Error Task: XXX" */
#define APP_DIAG_SHOW_FAULT     1U   /* 1 = 复位后停在故障界面等人工复位 */

/* 复位原因：由 App_Watchdog_ResetCause() 给出，App_Diag_Init() 消费 */
#define APP_DIAG_REASON_NONE        0U   /* 上电 / 按 NRST / 软复位，不是故障 */
#define APP_DIAG_REASON_IWDG        1U   /* 独立看门狗咬的 */
#define APP_DIAG_REASON_OVERDUE     2U   /* 任务超期，主动复位（后续步骤用） */
#define APP_DIAG_REASON_HARDFAULT   3U   /* 硬件异常（后续步骤用） */

/* 任务编号，Init 任务不在列表里：它是一次性任务，osThreadExit() 之后不再打点。 */
typedef enum {
    APP_DIAG_TASK_LED = 0,
    APP_DIAG_TASK_LCD,
    APP_DIAG_TASK_DHT11,
    APP_DIAG_TASK_BLUETOOTH,
    APP_DIAG_TASK_HUMID,
    APP_DIAG_TASK_WARM,
    APP_DIAG_TASK_FAN,
    APP_DIAG_TASK_WATCHDOG,
    APP_DIAG_TASK_SIZE      // 任务数量
} App_Diag_TaskId_t;

typedef struct {
    uint32_t magic;                       /* 本区有效标记，必须等于 APP_DIAG_MAGIC */
    uint32_t boot_cnt;                    /* 复位次数：冷启动为 1，每次复位 +1 */
    uint32_t last_blame;                  /* 上一轮的故障任务编号，APP_DIAG_NO_BLAME = 无 */
    uint32_t last_age;                    /* 上一轮元凶沉默的毫秒数 */
    uint32_t kick[APP_DIAG_TASK_SIZE];    /* 每个任务最后一次打点的时刻 */

    /* 本轮运行时的"轻警告"，和 last_* 分开存：
     * last_* 是上一次复位的定论，跨复位保留，
     * warn_* 是随时会消失的实时状态，任务恢复打卡就该清掉 */
    uint32_t warn_id;                     /* 当前超期、但还没到复位阈值的任务 */
    uint32_t warn_age;                    /* 它已经沉默了多少毫秒 */
} App_Diag_Zone_t;

/* App_Diag_Scan() 的结论，直接驱动喂狗任务的动作 */
typedef enum {
    APP_DIAG_RET_OK = 0,    /* 全部任务按期报到 */
    APP_DIAG_RET_WARN,      /* 有任务超期，还能观察，继续喂狗 */
    APP_DIAG_RET_FATAL      /* 超期已达复位阈值，必须停止喂狗 */
} App_Diag_Ret_t;

extern App_Diag_Zone_t g_diag;

void           App_Diag_Init(uint32_t reset_cause);
void           App_Diag_Kick(App_Diag_TaskId_t id);
App_Diag_Ret_t App_Diag_Scan(void);

void           App_Diag_TickIsr(void);
uint32_t       App_Diag_GetNow(void);
uint32_t       App_Diag_GetBootCount(void);
uint32_t       App_Diag_GetWarnAge(void);
uint32_t       App_Diag_GetWarnId(void);
char *         App_Diag_GetTaskName(uint32_t id);

#endif /* APP_DIAG_H */
