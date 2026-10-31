#include "FreeRTOS.h"
#include "task.h"

#include "DHT11.h"
#include "bsp_dht11.h"    /* DHT11 引脚：读写 + 方向切换（BSP 层直调 HAL） */
#include "DWT.h"

/* 1 us 对应的 CPU 周期数（72 MHz → 72）。
 * 用于把"高电平持续了多少 us"换算成 DWT 周期阈值。 */
#define DHT11_CYCLES_PER_US   (SystemCoreClock / 1000000U)

//  复位DHT11（主机拉低总线启动通信）
void DHT11_Rst(void) {
    Bsp_Dht11_SetOutput();      // 设置为输出模式
    DHT11_Low;                  // 拉低数据线
    HAL_Delay(20);        // 保持低电平至少18ms（推荐20ms）
    DHT11_High;                 // 释放总线，拉高
    Delay_Us(30);            // 等待10~35us（典型13us）
}

/**
 * @brief  等待DHT11响应（拉低信号）
 * @param  无
 * @retval uint8_t: 1-未检测到，0-检测到存在
 */
uint8_t DHT11_Check(void) {
    uint8_t retry = 0;
    Bsp_Dht11_SetInputPullup();         // 设置为输入模式

    // 等待DHT11拉低总线（低电平有效，表示响应）
    while (Bsp_Dht11_Read() && retry < 100) {
        retry++;
        Delay_Us(1);
    }
    if (retry >= 100) return 1;   // 超时，未检测到

    retry = 0;
    // 等待DHT11释放总线（拉高），表示响应结束
    while (!Bsp_Dht11_Read() && retry < 100) {
        retry++;
        Delay_Us(1);
    }
    if (retry >= 100) return 1;   // 超时

    return 0;   // 成功检测到
}

/**
 * @brief  从DHT11读取一个位（数据位）
 * @param  无
 * @retval uint8_t: 读取的位值（0或1）
 * @note   不用"固定延时40us后采样"（那种做法只要延时被中断打断>30us 就会把'1'读成'0'），
 *         改为直接测量高电平的持续时间：26~28us = '0'，70us = '1'，阈值取中间的 50us。
 *         这样即使中途被中断推迟了几个 us，测量结果依然落在正确的一侧，抗抖动能力强得多。
 */
uint8_t DHT11_Read_Bit(void) {
    uint8_t retry = 0;
    uint32_t start, width;

    // 等待总线变为低电平（每一位都以 50us 低电平开头）
    while (Bsp_Dht11_Read() && retry < 100) {
        retry++;
        Delay_Us(1);
    }
    retry = 0;

    // 等待总线变为高电平（数据位开始）
    while (!Bsp_Dht11_Read() && retry < 100) {
        retry++;
        Delay_Us(1);
    }

    // 测量本次高电平的持续时间（单位：CPU 周期）
    start = DWT->CYCCNT;
    retry = 0;
    while (Bsp_Dht11_Read() && retry < 100) {
        retry++;
        Delay_Us(1);
    }
    width = DWT->CYCCNT - start;    // 32 位无符号相减，自动处理计数器回绕

    return (width > DHT11_CYCLES_PER_US * 50U) ? 1 : 0;
}

/**
 * @brief  从DHT11读取一个字节（8位）
 * @param  无
 * @retval uint8_t: 读到的字节数据
 */
uint8_t DHT11_Read_Byte(void) {
    uint8_t i, dat = 0;
    for (i = 0; i < 8; i++) {
        dat <<= 1;                      // 左移一位，空出最低位
        dat |= DHT11_Read_Bit();        // 将读取的位填入最低位
    }
    return dat;
}

/**
 * @brief  从DHT11读取一次完整的数据（温度+湿度）
 * @param  temp: 指向存储温度值的变量（范围0~50°）
 * @param  humi: 指向存储湿度值的变量（范围20%~90%）
 * @retval uint8_t: 0-正常读取，1-读取失败
 * @note   应答信号与 40 位数据期间（约 5 ms）全程关中断。
 *         DHT11 的位宽只有 26us/70us 这个量级，而 FreeRTOS 每 1 ms 就有一次
 *         SysTick / TIM1 中断（还可能伴随 PendSV 任务切换，开销可达 10~20us），
 *         一旦落在采样窗口内就会误码；更糟的是本任务由 TIM1 信号量唤醒，
 *         读的相位和 1ms 节拍是锁定的，同一个位可能每次都被打断 → 永远校验失败。
 *         关中断代价：每秒损失约 5 个 tick（0.5% 时间误差），对本项目无影响。
 *         注意 DHT11_Rst() 里 20ms 的 osDelay 必须留在临界区外——
 *         它依赖 TIM3 中断推进的 HAL 时基。
 */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi) {
    uint8_t buf[5];
    uint8_t i;
    uint8_t result = 1;                 // 默认按"失败"处理

    DHT11_Rst();                        // 复位并准备通信（内含 20ms 延时，必须在临界区外）

    /* 应答信号 + 40 位数据全程挡中断（约 4~5ms）。
     * taskENTER_CRITICAL 走 BASEPRI：本工程最低的中断优先级是 5
     * （configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY），所以 TIM1/SysTick/TIM6/uart/DMA
     * 全部被挡住，等效于 __disable_irq()。自带嵌套计数，比手写 PRIMASK 更稳。 */
    taskENTER_CRITICAL();               // ↓↓↓ 屏蔽中断，保护位时序 ↓↓↓

    if (DHT11_Check() == 0)             // 检测到DHT11响应
    {
        for (i = 0; i < 5; i++)         // 读取40位数据（5字节）
        {
            buf[i] = DHT11_Read_Byte();
        }
        // 校验和验证：前4字节之和（取低8位）等于第5字节
        if (((buf[0] + buf[1] + buf[2] + buf[3]) & 0xFF) == buf[4]) {
            *humi = buf[0];             // 湿度整数部分
            *temp = buf[2];             // 温度整数部分
            result = 0;                 // 成功
        }
    }

    taskEXIT_CRITICAL();                // ↑↑↑ 恢复中断 ↑↑↑

    return result;
}

/**
 * @brief  初始化DHT11的GPIO引脚，并检测设备
 * @param  无
 * @retval uint8_t: 1-不存在，0-存在
 */
uint8_t DHT11_Init(void) {
    DWT_Init();
    Bsp_Dht11_Init();           /* 推挽输出 + 空闲态拉高 */

    DHT11_Rst();                // 复位
    return DHT11_Check();       // 返回检测结果
}
