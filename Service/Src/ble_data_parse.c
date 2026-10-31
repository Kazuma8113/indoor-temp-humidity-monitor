#include "ble_data_parse.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "bsp_usart1.h"    /* 串口收发（Bsp 层） */
#include "cmsis_os.h"      /* osDelay */

/* ==================== 静态缓冲 ==================== */

/* 发送缓冲：DMA 在函数返回后还在搬数据，用栈缓冲会搬到垃圾 */
static uint8_t tx_buf[BSP_USART1_BUF_SIZE];

/* 中断里取字节的中转缓冲：一次取完整圈，用完即弃 */
static uint8_t rx_buf[BSP_USART1_BUF_SIZE];

/* 累积缓冲：装"还没凑齐的半个帧"（含 ']' 之后多出来的下一帧开头） */
static char joint_frame[BSP_USART1_BUF_SIZE];
static uint16_t joint_len;

/* 成帧结果：一帧凑齐后先放这里，等任务来取 */
static char frame_buf[BSP_USART1_BUF_SIZE];
static uint16_t frame_len; // 帧长度
static uint16_t rest_len;  // 剩余字节长度
static volatile uint8_t frame_ready;      /* ISR 置 1，任务清 0 */
static volatile uint8_t frame_bad;        /* ISR 置 1：刚扔掉一个废帧 */

/* ==================== 成帧：协议复位 ==================== */
/* 丢弃当前累积的半个帧（串口出错、协议重启时用） */
static void FrameRx_Reset(void) {
    taskENTER_CRITICAL();
    joint_len = 0U;
    joint_frame[0] = '\0';
    frame_ready = 0U;
    frame_bad = 0U;
    taskEXIT_CRITICAL();
}

/* ==================== 成帧：从 joint_frame 抠一帧 ==================== */
/* 把 joint_frame 里第一个完整的 "[...]" 搬进 frame_buf，等任务来取。
 * 返回 1 = 抠到一帧（frame_ready 已置 1），0 = 还没凑齐 / 上一帧还没被取走。
 *
 * 调用者负责互斥：USART1 中断里直接调（ISR 本身不会被同级打断），
 * 任务里调必须先 taskENTER_CRITICAL()。 */
static uint8_t FrameRx_Promote(void) {
    char *frame_end;

    if (frame_ready == 1U) return 0U;      /* 上一帧还没被任务取走，原地等 */
    if (joint_len < 2U) return 0U;         /* 连 "[]" 都凑不出来 */

    frame_end = strchr(joint_frame, ']');
    if (frame_end == NULL) return 0U;      /* 帧还没收全，等下一段 */

    frame_len = (uint16_t) (frame_end - joint_frame) + 1U;   /* 长度要 +1 才能把 ']' 本身算进去 */
    rest_len = (uint16_t) (joint_len - frame_len);           /* ']' 之后还剩多少字节 */

    memcpy(frame_buf, joint_frame, frame_len);
    frame_buf[frame_len] = '\0';
    frame_ready = 1U;

    /* 把 ']' 之后多出来的字节前移，留给下一帧（支持一段里含多帧） */
    memmove(joint_frame, frame_end + 1, rest_len);
    joint_len = rest_len;
    joint_frame[joint_len] = '\0';          /* 必须补：否则下次 strchr 会读越界 */

    return 1U;
}

/* ==================== 成帧：中断上下文 ==================== */
/* data_buf : 取字节的中转缓冲（rx_buf），容量 >= BSP_USART1_BUF_SIZE
 * len      : 该缓冲的容量。Bsp_Usart1_TakeBytes 会把它改写成"这次真正取到的字节数" */
static void FrameRx_Feed(uint8_t *data_buf, uint16_t len) {
    uint16_t size = len;
    uint16_t i;
    char *frame_start;

    if (data_buf == NULL || len == 0U) return;

    /* 有新字节才追加。没新字节也不能提前 return —— joint_frame 里可能还压着上一轮
     * 没来得及交付的帧（一次收到多帧时，一次 IDLE 只交一帧）。 */
    if (Bsp_Usart1_TakeBytes(data_buf, &size)) {
        /* ---------- 1.将数据追加到累积缓冲 ---------- */
        for (i = 0; i < size; i++) {
            /* 坏帧：攒满整块缓冲还等不到 ']'，这条帧永远不完整了 */
            if (joint_len >= BSP_USART1_BUF_SIZE - 1U) {
                joint_len = 0U;
                joint_frame[0] = '\0';
                frame_bad = 1U;
                return;
            }
            joint_frame[joint_len++] = (char) data_buf[i];
        }
        joint_frame[joint_len] = '\0';

        /* ---------- 2.找帧头 ---------- */
        frame_start = strchr(joint_frame, '[');
        /* 没有帧头 = 这一批字节根本不是帧（JDY-33 日志、AT 回显、上电毛刺）。
         * 这不是"坏帧"：回 [FRAME ERROR!] 只会和真正的 ACK 抢 TX，直接丢弃。 */
        if (frame_start == NULL) {
            joint_len = 0U;
            joint_frame[0] = '\0';
            return;
        }
        /* 有帧头但不是第一个数据 */
        if (frame_start != joint_frame) {
            joint_len = joint_len - (uint16_t) (frame_start - joint_frame);
            memmove(joint_frame, frame_start, (size_t) joint_len + 1U);   /* 加1把 '\0' 算进去 */
        }
    }

    /* ---------- 3.从 joint_frame 里抠一帧 ---------- */
    FrameRx_Promote();
}

/* ==================== 成帧：任务上下文 ==================== */
/* 返回值：1 = 完整帧，0 = 没帧，-1 = 问题帧。长度走 data_len。 */
static int FrameRx_Read(char *data_buf, uint16_t buf_len, uint16_t *data_len) {
    int frame_state;

    /* 参数非法属于编程错误，按"没帧"返回（0），不要返回 -1 —— 那会凭空发一条 NACK */
    if (data_buf == NULL || data_len == NULL || buf_len < 2U) return 0;
    *data_len = 0U;

    /* 拷贝期间挡住 IDLE 中断，否则会抄到半新半旧的数据 */
    taskENTER_CRITICAL();

    if (frame_ready) {
        uint16_t n = frame_len;

        if (n > (uint16_t) (buf_len - 1U)) {
            n = (uint16_t) (buf_len - 1U);
        }
        memcpy(data_buf, frame_buf, (size_t) n);
        data_buf[n] = '\0';

        *data_len = n;
        frame_ready = 0U;
        frame_bad = 0U;      /* 手机重传可能短时间内在坏帧之后发送一段完整帧 */
        frame_state = 1;

        /* 立刻把 joint_frame 里的下一帧顶上来：否则它要等到下一个字节到达才会被解析，
         * 手机停等时会一直等不到 ACK。 */
        FrameRx_Promote();
    } else if (frame_bad) {
        frame_bad = 0U;
        frame_state = -1;
    } else {
        frame_state = 0;
    }

    taskEXIT_CRITICAL();

    return frame_state;
}

/* ==================== 初始化 ==================== */
void Bluetooth_Init(void) {
    FrameRx_Reset();        /* 清成帧器的累积状态 */
    Bsp_Usart1_Init();      /* USART1 + DMA 循环接收 + IDLE / 错误中断 */
}

/* 空闲中断入口：从 Bsp 取出这一批字节，直接喂给成帧器 */
void Bluetooth_OnIdle(void) {
    FrameRx_Feed(rx_buf, BSP_USART1_BUF_SIZE);
}

/* ==================== 发送 ==================== */
/* 发一帧：直接交给 DMA，然后立刻返回。
 * 线路忙（上一帧还没发完）就让出 10ms 再试一次，仍失败返回 0，由调用方降级
 * 1 ：表示发送成功
 * 0 ：表示发送失败 */
uint8_t Bluetooth_SendHandler(const uint8_t *buf, uint16_t len) {
    if (buf == NULL || len == 0U || len > BSP_USART1_BUF_SIZE) return 0U;

    if (Bsp_Usart1_IsTxDone() == 0U) {     /* 上一帧还在搬，先等 */
        osDelay(10);                        /* 115200 下 22 字节约 1.9ms，10ms 足够 */
        if (Bsp_Usart1_IsTxDone() == 0U) return 0U;
    }

    /* DMA 在函数返回后还在搬数据，用栈缓冲会搬到垃圾，先拷进静态缓冲 */
    memcpy(tx_buf, buf, len);

    return Bsp_Usart1_SendDma(tx_buf, len);
}

/* ==================== 接收 ==================== */

/* 一次性握手：手机发 [SUCCESSFUL CONNECTION!] → MCU 回一条即完成 */
static uint8_t Ble_IsHandshake(const char *frame) {
    return (strstr(frame, "SUCCESSFUL CONNECTION") != NULL) ? 1U : 0U;
}

/* 解析传来的命令 */
static Ble_Status_t Ble_Parse(const char *frame,
                              Ble_Threshold_t *threshold,
                              Ble_LedCmd_t *led_cmd) {
    /* 阈值包。%hd 对应 int16_t：手机允许零下温度，用 %hu 会把 -40 读成 65496。
     * 范围和上下限顺序由 APP 的 ThresholdActivity 负责，这里不再重复校验。 */
    if (sscanf(frame,
               "[TEMP_MIN:%hd;TEMP_MAX:%hd;HUM_MIN:%hd;HUM_MAX:%hd]",
               &threshold->Temp_Min, &threshold->Temp_Max,
               &threshold->Hum_Min, &threshold->Hum_Max) == 4) {
        return BT_DHT_STATUS_SUCCESS;
    }

    if (strstr(frame, "LED_ON") != NULL) {
        *led_cmd = BT_LED_CMD_ON;
        return BT_LED_STATUS_SUCCESS;
    }

    if (strstr(frame, "LED_OFF") != NULL) {
        *led_cmd = BT_LED_CMD_OFF;
        return BT_LED_STATUS_SUCCESS;
    }

    return BT_STATUS_PARSE_ERROR;   /* 数据解析失败 */
}

/* 接收处理：取一帧，解析它。 */
Ble_Status_t Bluetooth_ReceiveHandler(Ble_Threshold_t *th, Ble_LedCmd_t *led_cmd) {
    /* 本帧的副本放在栈上：FrameRx_Read() 在临界区里只做这次拷贝，
     * 出了临界区 sscanf 想跑多久跑多久，不占着 USART1 中断。 */
    char frame[BSP_USART1_BUF_SIZE];
    int frame_state;
    uint16_t len;

    if (th == NULL || led_cmd == NULL) return BT_STATUS_BUSY;

    *led_cmd = BT_LED_CMD_NONE;          /* 默认：本帧没有 LED 指令 */

    /* 成帧器三态返回：1 = 完整帧，0 = 没帧（只有噪声），-1 = 问题帧（已丢弃） */
    frame_state = FrameRx_Read(frame, sizeof(frame), &len);

    if (frame_state == 0) return BT_STATUS_BUSY;          /* 没帧：只有噪声：一个字节都不回 */
    if (frame_state < 0) return BT_STATUS_FRAME_ERROR;    /* 废帧：让手机立刻重传，别干等它的超时 */

    /* ---- 以下一定是完整帧 ---- */
    if (len < 3U) return BT_STATUS_DATA_TOO_SHORT;        /* 空帧："[]" */

    /* 握手包判断 */
    if (Ble_IsHandshake(frame)) return BT_STATUS_HANDSHAKE;

    /* 普通指令：解析结果写进调用方给的出参，状态由调用方决定怎么回话 */
    return Ble_Parse(frame, th, led_cmd);
}

/* 根据状态回复 */
void Bluetooth_ReplyByStatus(Ble_Status_t status) {
    char temp[64];
    uint16_t len;

    switch (status) {
        case BT_STATUS_HANDSHAKE:
            len = (uint16_t) sprintf(temp, "[SUCCESSFUL CONNECTION!]\n");
            break;
        case BT_DHT_STATUS_SUCCESS:
        case BT_LED_STATUS_SUCCESS:
            len = (uint16_t) sprintf(temp, "[SET OK!]\n");
            break;
        case BT_STATUS_FRAME_ERROR:
            len = (uint16_t) sprintf(temp, "[FRAME ERROR!]\n");
            break;
        case BT_STATUS_DATA_TOO_SHORT:
            len = (uint16_t) sprintf(temp, "[DATA TOO SHORT!]\n");
            break;
        case BT_STATUS_PARSE_ERROR:
            len = (uint16_t) sprintf(temp, "[PARSE ERROR!]\n");
            break;

        case BT_STATUS_BUSY:
        default:
            return;
    }

    /* ACK 发失败也不重发：手机端没收到 ACK 会按自己的超时重传上一条指令，
     * 这里再补一次只会让对端收到重复应答。 */
    (void) Bluetooth_SendHandler((const uint8_t *) temp, len);
}
