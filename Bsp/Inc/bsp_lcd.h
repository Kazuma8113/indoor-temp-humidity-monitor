#ifndef BSP_LCD_H
#define BSP_LCD_H

/* ============================================================================
 * 板级封装：LCD 的 SPI 总线（当前是 SPI3）
 * ========================================================================== */

#include "main.h"

/* 指向 LCD 使用的 SPI 句柄，定义在 Bsp/Src/bsp_lcd.c
 * 宏定义展开后只是一次指针取值，编译器会放进寄存器复用，不会太占用编译时间 */
extern SPI_HandleTypeDef *const Bsp_Lcd_Spi;

/* 总线初始化：使能 SPI + 切换成单线发送模式。*/
void Bsp_Lcd_BusInit(void);

/* ---- 三根控制脚：片选 / 数据-指令选择 / 背光 ---- */
void Bsp_Lcd_CsHigh(void);
void Bsp_Lcd_CsLow(void);
void Bsp_Lcd_DcCommand(void);      /* DC = 低：指令传输 */
void Bsp_Lcd_DcData(void);         /* DC = 高：数据传输 */
void Bsp_Lcd_Backlight(uint8_t state);

#endif /* BSP_LCD_H */
