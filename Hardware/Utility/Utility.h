#ifndef PROJECT_UTILITY_H
#define PROJECT_UTILITY_H

#include "main.h"
unsigned int numlen(unsigned int num);
void float_round_trim(float value, char *str, int buf_size, int precision);

/* 异常指示：死循环闪烁板载 LED（PC13，低电平点亮）。
 * 纯忙等 + 直接写 GPIO，不使用任何 RTOS API，
 * 因此在 malloc 失败钩子 / 栈溢出钩子 / HardFault 等异常上下文里都能安全调用。 */
void Error_Blink(uint32_t on_ms, uint32_t off_ms);

/* 异常指示（带编号）：先长亮一次作为"组起始标记"，随后短闪 n 次，再停顿，循环。
 * 用于在异常上下文里报告"是哪个任务"：n 即任务编号（见 freertos.c 的任务名表）。 */
void Error_BlinkCode(uint32_t n);

#endif //PROJECT_UTILITY_H
