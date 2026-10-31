#ifndef BLE_DATA_PARSE_H
#define BLE_DATA_PARSE_H

#include <stdint.h>

/* ============================================================================
 * 蓝牙协议（Service 层）
 *
 * 本层只做三件事：把字节流切成帧、把帧解析成结构体、按结果回一条应答。
 * ========================================================================== */

/* 蓝牙阈值帧解析出来的字段。
 * 这是协议层的数据（对应 "[TEMP_MIN:%hd;TEMP_MAX:%hd;HUM_MIN:%hd;HUM_MAX:%hd]"），
 * 必须是有符号的：Android 端 ThresholdStore 的温度范围是 -40~85*/
typedef struct {
    int16_t Temp_Min;
    int16_t Temp_Max;
    int16_t Hum_Min;
    int16_t Hum_Max;
} Ble_Threshold_t;

/* 手机下发的 LED 指令。
 * 解析与执行分开：Service 只"读懂意图"，改 LED_State 是 App 的事。 */
typedef enum {
    BT_LED_CMD_NONE = 0,     /* 本帧没有 LED 指令 */
    BT_LED_CMD_ON,           /* 手机要求点亮 */
    BT_LED_CMD_OFF           /* 手机要求熄灭 */
} Ble_LedCmd_t;

typedef enum {
    BT_STATUS_BUSY,              // 串口接收繁忙（本轮没有完整帧）
    BT_DHT_STATUS_SUCCESS,       // 温湿度阈值接收并解析成功
    BT_LED_STATUS_SUCCESS,       // LED 指令接收并解析成功
    BT_STATUS_HANDSHAKE,         // 握手包，已完成应答
    BT_STATUS_FRAME_ERROR,       // 数据帧格式错误
    BT_STATUS_DATA_TOO_SHORT,    // 数据长度不足
    BT_STATUS_PARSE_ERROR        // 数据解析失败
} Ble_Status_t;

/* ---- 对外只有这 4 个接口 ---- */

/* 初始化蓝牙串口（USART1 + DMA 循环接收 + IDLE 中断）。上电调用一次。 */
void Bluetooth_Init(void);

/* 空闲中断入口（**中断上下文**，由 USART1_IRQHandler 调用）。*/
void Bluetooth_OnIdle(void);

/* 发一帧（不排队）。线路忙就让出 10ms 再试一次，最多阻塞 10ms。
 * buf 内容会被拷到内部静态缓冲再交给 DMA，调用后可以立刻复用 buf。
 * 返回 1 = 已交给 DMA 发出，0 = 没发出去（参数非法 / 线路一直忙 / DMA 启动失败）。
 * 返回 1 只代表"开始发了"，不代表对端收到 —— 那是停等协议 ACK 的事。
 *
 * 只能在**任务上下文**调用：内部会 osDelay()，且 tx_buf 是单实例。
 * 目前只有 App_Bluetooth_Run 一个任务使用；若将来多任务要发，必须先加互斥。 */
uint8_t Bluetooth_SendHandler(const uint8_t *buf, uint16_t len);

/* 取一帧并解析。一次只交一帧；内部会顺手把 joint_frame 里压着的下一帧顶上来，
 * 所以连续多帧不会被"卡住等下一个字节"。
 *   th       : 出参，本帧是阈值包时给出 4 个字段（不是阈值包则不动）
 *   led_cmd  : 出参，本帧若是 LED 指令则给出意图，否则 BT_LED_CMD_NONE
 * 返回状态由调用方决定怎么回话（见 Bluetooth_ReplyByStatus）。 */
Ble_Status_t Bluetooth_ReceiveHandler(Ble_Threshold_t *th, Ble_LedCmd_t *led_cmd);

/* 按解析结果回一条应答帧。BT_STATUS_BUSY 时不发任何东西。
 * 同样只能在任务上下文调用（最终会走 Bluetooth_SendHandler）。 */
void Bluetooth_ReplyByStatus(Ble_Status_t status);

#endif /* BLE_DATA_PARSE_H */

