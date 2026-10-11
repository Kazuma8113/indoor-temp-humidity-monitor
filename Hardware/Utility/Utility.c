#include "Utility.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* 返回十进制位数，纯整数实现，不使用 sprintf。
 * 原因：sprintf 会走 newlib 的 _svfprintf_r（实测单帧 384 B），
 * 在 512 B 的任务栈上调用会直接栈溢出。 */
unsigned int numlen(unsigned int num){
    unsigned int n = 1;
    while (num >= 10U) {
        num /= 10U;
        n++;
    }
    return n;
}

// 将 float 转成字符串，四舍五入到指定小数位，并去掉末尾多余的 0
void float_round_trim(float value, char *str, int buf_size, int precision)
{
    // 四舍五入到指定小数位
    float rounded = roundf(value * powf(10, precision)) / powf(10, precision);

    snprintf(str, buf_size, "%.*f", precision, rounded);

    // 去掉末尾的 0
    char *p = str + strlen(str) - 1;
    while (*p == '0' && p > str) {
        *p = '\0';
        p--;
    }

    // 如果最后是小数点，也去掉
    if (*p == '.')  *p = '\0';
}

// 异常指示：死循环闪烁板载 LED（PC13，低电平点亮）
// 不调用任何 RTOS API，也不依赖中断/系统时基，可在异常上下文里安全使用
// 说明：忙等系数按 168 MHz（F407）标定；F103 为 72 MHz 时用 7000。
#define BUSY_LOOP_PER_MS  16000U

void Error_Blink(uint32_t on_ms, uint32_t off_ms)
{
    for (;;) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);   // 点亮
        for (volatile uint32_t i = 0; i < on_ms * BUSY_LOOP_PER_MS; i++) {
        }
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);     // 熄灭
        for (volatile uint32_t i = 0; i < off_ms * BUSY_LOOP_PER_MS; i++) {
        }
    }
}

/* 异常指示（带编号）：一次长亮 = 组起始，随后短闪 n 次 = 编号，再停顿后重复。
 * 同样零 RTOS 依赖，可在任何异常上下文安全调用。 */
void Error_BlinkCode(uint32_t n)
{
    if (n == 0U) {
        n = 1U;
    }

    for (;;) {
        /* 组起始标记：长亮 1.2 s */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        for (volatile uint32_t i = 0; i < 1200U * BUSY_LOOP_PER_MS; i++) {
        }
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        for (volatile uint32_t i = 0; i < 600U * BUSY_LOOP_PER_MS; i++) {
        }

        /* 短闪 n 次（每次 200 ms 亮 / 200 ms 灭） */
        for (uint32_t k = 0; k < n; k++) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
            for (volatile uint32_t i = 0; i < 200U * BUSY_LOOP_PER_MS; i++) {
            }
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
            for (volatile uint32_t i = 0; i < 200U * BUSY_LOOP_PER_MS; i++) {
            }
        }

        /* 组间隔 1.5 s，便于下一组重新计数 */
        for (volatile uint32_t i = 0; i < 1500U * BUSY_LOOP_PER_MS; i++) {
        }
    }
}

