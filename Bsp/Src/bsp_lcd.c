#include "bsp_lcd.h"
#include "spi.h"        /* 只有这里知道 LCD 挂在哪个 SPI 上 */
#include "bsp_pin.h"    /* CS / DC / 背光 三根脚的坐标 */

/* 换成 SPI2 / SPI4 只改这一行 */
SPI_HandleTypeDef *const Bsp_Lcd_Spi = &hspi3;

void Bsp_Lcd_BusInit(void) {
    /* 面板只往一个方向写数据，用单线发送模式；
     * 数据搬运走驱动里的寄存器紧循环（见 bsp_lcd.h 的边界说明）。 */
    __HAL_SPI_ENABLE(Bsp_Lcd_Spi);      /* 使能 SPI */
    SPI_1LINE_TX(Bsp_Lcd_Spi);          /* 单线发送 */
}

/* ---- 三根控制脚 ---- */
void Bsp_Lcd_CsHigh(void) {
    HAL_GPIO_WritePin(LcdPin.CS.Port, LcdPin.CS.Pin, GPIO_PIN_SET);       /* 片选无效 */
}

void Bsp_Lcd_CsLow(void) {
    HAL_GPIO_WritePin(LcdPin.CS.Port, LcdPin.CS.Pin, GPIO_PIN_RESET);     /* 片选有效 */
}

void Bsp_Lcd_DcCommand(void) {
    HAL_GPIO_WritePin(LcdPin.DC.Port, LcdPin.DC.Pin, GPIO_PIN_RESET);     /* 低 = 指令 */
}

void Bsp_Lcd_DcData(void) {
    HAL_GPIO_WritePin(LcdPin.DC.Port, LcdPin.DC.Pin, GPIO_PIN_SET);       /* 高 = 数据 */
}

void Bsp_Lcd_Backlight(uint8_t state) {
    /*  背光高有效，且必须配成 GPIO_Output —— 配成 Analog 会全黑 */
    HAL_GPIO_WritePin(LcdPin.Backlight.Port, LcdPin.Backlight.Pin,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
