#ifndef BSP_PIN_H
#define BSP_PIN_H

#include "main.h"       /* GPIO_TypeDef / GPIO_PIN_n */

/* 一根脚：端口 + 引脚号 */
typedef struct {
    GPIO_TypeDef *Port;
    uint16_t      Pin;
} BspPin_t;

/* ==================== 多脚器件（按 器件.脚名 访问）==================== */

/* ---- ST7789 SPI 屏（SPI3 + 3 根控制脚）---- */
typedef struct {
    BspPin_t SCK;          /* SPI3 SCK（复用，仅登记） */
    BspPin_t SDA;          /* SPI3 MOSI（复用，仅登记） */
    BspPin_t CS;           /* 片选，低有效 */
    BspPin_t DC;           /* 数据/指令选择 */
    BspPin_t Backlight;    /* 背光，高有效 */
} LcdPin_t;
extern const LcdPin_t LcdPin;

/* ---- TB6612 电机驱动（调速走 TIM2_CH1，不在这里）---- */
typedef struct {
    BspPin_t AIN1;         /* 方向 A */
    BspPin_t AIN2;         /* 方向 B */
    BspPin_t STBY;         /* 使能，高有效 */
} MotorPin_t;
extern const MotorPin_t Motor;

/* ---- 加湿器 ---- */
typedef struct {
    BspPin_t CTRL;         /* 控制脚，高电平开 */
    BspPin_t GND;          /* 对接的 GND 脚，拉低 */
} HumidPin_t;
extern const HumidPin_t Humid;

/* ==================== 单脚器件（保持平铺）==================== */

extern const BspPin_t LedPin;      /* PC13，低电平点亮 */
extern const BspPin_t Dht11Pin;    /* PA11，单总线数据脚 */
extern const BspPin_t WarmPin;     /* PD10，低电平有效 */

#endif /* BSP_PIN_H */
