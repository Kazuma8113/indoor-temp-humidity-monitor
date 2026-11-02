#include "app_diag.h"

/* 放进 .noinit 段：
 *   - section 属性把变量定位到链接脚本里的 .noinit（绝对地址 0x10000000，CCM RAM）
 *   - used 防止 -ffunction-sections/-fdata-sections 下被当垃圾回收
 * 这段不会被 startup 清零，也不占 Flash 空间（NOLOAD）。
 * 放 CCM 而不放主 RAM：主 RAM 只有 128KB，且记录区必须钉死绝对地址
 * （BL 与 APP 是两个独立链接脚本，.bss 大小不同），钉在 0x20018000 会让
 * 链接器把中间的 96KB 空洞算进使用率，报告从 32KB 跳到 98KB。
 * CCM 是独立 64KB、CPU 零等待访问、不参与主 RAM 统计。 */
__attribute__((section(".noinit"), used)) App_Diag_Zone_t g_diag;

// 1ms 自由计数，由 App_Heartbeat() 在 TIM1 中断里递增
static volatile uint32_t diag_now_ms;

/* ==================== 每个任务的超期预算 ==================== */
typedef struct {
    uint16_t budget_ms;   // 允许它沉默多久
    uint8_t  reset;       // 是否复位
} App_Diag_Budget_t;

#define APP_DIAG_FATAL_MULT  3U   /* 超期到 3 倍预算 = 判定不可恢复 */

// 任务超期预算表
static const App_Diag_Budget_t task_budget[APP_DIAG_TASK_SIZE] = {
    [APP_DIAG_TASK_LED]       = {  300U, 1U },
    [APP_DIAG_TASK_LCD]       = {  300U, 1U },
    [APP_DIAG_TASK_DHT11]     = { 1500U, 1U },
    [APP_DIAG_TASK_BLUETOOTH] = {  600U, 1U },

    /* 这三个的打卡挂在 DHT11 的成功上：EVENT_DATA_COMPLETE 只在读成功时才置位，
     * 所以 DHT11 接线松了会让它们集体沉默。这种情况复位救不了（重初始化后照样读不到），
     * 只会变成"复位 → 再失败 → 再复位"的闪屏死循环。
     * 因此它们只报警不复位：屏上持续挂着告警，由人去判断是接线还是器件问题。 */
    [APP_DIAG_TASK_HUMID]     = { 3000U, 0U },
    [APP_DIAG_TASK_WARM]      = { 3000U, 0U },
    [APP_DIAG_TASK_FAN]       = { 3000U, 0U },

    /* reset 对它其实不生效（它刚给自己打完卡就扫），写 1 只是保持表意一致 */
    [APP_DIAG_TASK_WATCHDOG]  = {  300U, 1U },
};

/* 任务短名：给 LCD 告警行用*/
static char *const task_name[APP_DIAG_TASK_SIZE] = {
        [APP_DIAG_TASK_LED]       = "LED",
        [APP_DIAG_TASK_LCD]       = "LCD",
        [APP_DIAG_TASK_DHT11]     = "DHT11",
        [APP_DIAG_TASK_BLUETOOTH] = "BT",
        [APP_DIAG_TASK_HUMID]     = "HUMID",
        [APP_DIAG_TASK_WARM]      = "WARM",
        [APP_DIAG_TASK_FAN]       = "FAN",
        [APP_DIAG_TASK_WATCHDOG]  = "WDOG",
};

/* 任务每跑完一轮调一次，记下当前时刻。
 * 必须放在"活干完之后"，防止任务卡在等事件时还在假装健康，检测会指错人。 */
void App_Diag_Kick(App_Diag_TaskId_t id) {
    if ((uint32_t) id < (uint32_t) APP_DIAG_TASK_SIZE) {
        g_diag.kick[id] = diag_now_ms;
    }
}

/* 复位后最早调用（main() 进调度器之前）：按复位原因校验 magic、读回上一轮记录。 */
void App_Diag_Init(uint32_t reset_cause) {
    if (g_diag.magic != APP_DIAG_MAGIC) {
        /* magic 不对 = 冷启动，或者掉电后 RAM 已经丢了，或者结构体升级过。
         * 重新初始化结构体，避免读到掉电前的随机残留。 */
        g_diag.magic      = APP_DIAG_MAGIC;
        g_diag.boot_cnt   = 1U;
        g_diag.last_blame = APP_DIAG_NO_BLAME;
        g_diag.last_age   = 0U;
    } else {
        g_diag.boot_cnt++;

        if (reset_cause == APP_DIAG_REASON_NONE){
            /* 普通复位：不是故障，清掉上一轮的残留，避免下次误报。 */
            g_diag.last_blame = APP_DIAG_NO_BLAME;
            g_diag.last_age   = 0U;
        }
    }

    for (uint8_t i = 0U; i < (uint8_t)APP_DIAG_TASK_SIZE; i++) {
        g_diag.kick[i] = 0U;
    }

    /* warn 是"本轮实时状态"，上一轮的告警在复位后已经过期。 */
    g_diag.warn_id  = APP_DIAG_NO_BLAME;
    g_diag.warn_age = 0U;
}

/* 扫描一轮打卡记录，判定是否超期 */
App_Diag_Ret_t App_Diag_Scan(void) {
    /* 用局部变量获取快照，防止被 1ms 中断修改 */
    uint32_t now = diag_now_ms;
    uint32_t i;

    /* first 是"该怀疑谁"（按枚举顺序取最上游的那个，只用来显示）
     * fatal 是"该不该复位"（必须是 reset 的任务才作数） */
    uint32_t first_id  = APP_DIAG_NO_BLAME;
    uint32_t first_age = 0U;
    uint32_t fatal_id  = APP_DIAG_NO_BLAME;
    uint32_t fatal_age = 0U;

    for (i = 0U; i < (uint32_t) APP_DIAG_TASK_SIZE; i++) {
        uint32_t age    = now - g_diag.kick[i];
        uint32_t budget = (uint32_t) task_budget[i].budget_ms;

        if (age <= budget) continue;

        if (first_id == APP_DIAG_NO_BLAME) {
            first_id  = i;
            first_age = age;
        }

        if (task_budget[i].reset == 1U &&
            age >= budget * APP_DIAG_FATAL_MULT &&
            fatal_id == APP_DIAG_NO_BLAME) {
            fatal_id  = i;
            fatal_age = age;
        }
    }

    /* 复位判决优先：只要有一个"有权复位"的任务到了 3 倍预算，就不再观察了。 */
    if (fatal_id != APP_DIAG_NO_BLAME) {
        g_diag.warn_id  = fatal_id;
        g_diag.warn_age = fatal_age;

        /* 先把元凶写进保留区、再交给调用方停喂狗 —— 顺序不能反：
         * 停喂之后最多 348ms 就复位，来不及再补写这一次记录。 */
        g_diag.last_blame = fatal_id;
        g_diag.last_age   = fatal_age;

        return APP_DIAG_RET_FATAL;
    }

    /* 有任务超期，但都属于"还能观察"的范围（或者它本来就没有复位权） */
    if (first_id != APP_DIAG_NO_BLAME) {
        g_diag.warn_id  = first_id;
        g_diag.warn_age = first_age;

        return APP_DIAG_RET_WARN;
    }

    /* 全部按期报到：把上一轮的轻警告撤掉，LCD 上就不会挂着一个已经恢复的告警 */
    g_diag.warn_id  = APP_DIAG_NO_BLAME;
    g_diag.warn_age = 0U;

    return APP_DIAG_RET_OK;
}
/**************** 私有变量操作 ******************/
/* 1ms 中断里调（App_Heartbeat），进行计数 */
void App_Diag_TickIsr(void) { diag_now_ms++; }
/* 获取当前毫秒计数，和 kick[] 同一基准 */
uint32_t App_Diag_GetNow(void) {
    return diag_now_ms;
}

/* 当前是第几次启动，用来验证 .noinit 是否真的活过了复位 */
uint32_t App_Diag_GetBootCount(void) {
    return g_diag.boot_cnt;
}
/* 获取本轮的轻警告。id 为 APP_DIAG_NO_BLAME 表示此刻一切正常 */
uint32_t App_Diag_GetWarnId(void) {
    return g_diag.warn_id;
}

/* 获取本轮的轻警告年龄。0 表示此刻一切正常 */
uint32_t App_Diag_GetWarnAge(void) {
    return g_diag.warn_age;
}

/* 获取任务名 */
char *App_Diag_GetTaskName(uint32_t id) {
    if (id >= APP_DIAG_TASK_SIZE) return "?"; // 任务不存在
    return task_name[id];
}

