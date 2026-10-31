#include "bsp_pin.h"

/* ==================== 板级引脚表 ====================*/
/* ---- ST7789 SPI 屏 ---- */
const LcdPin_t LcdPin = {
        .SCK       = {GPIOB, GPIO_PIN_3},
        .SDA       = {GPIOB, GPIO_PIN_5},
        .CS        = {GPIOD, GPIO_PIN_11},
        .DC        = {GPIOD, GPIO_PIN_12},
        .Backlight = {GPIOD, GPIO_PIN_13},
};

/* ---- TB6612 电机驱动 ---- */
const MotorPin_t Motor = {
        .AIN1 = {GPIOE, GPIO_PIN_4},
        .AIN2 = {GPIOE, GPIO_PIN_5},
        .STBY = {GPIOE, GPIO_PIN_6},
};

/* ---- 加湿器 ---- */
const HumidPin_t Humid = {
        .CTRL = {GPIOF, GPIO_PIN_5},
        .GND  = {GPIOF, GPIO_PIN_6},
};

/* ---- 单脚器件 ---- */
const BspPin_t LedPin = {GPIOC, GPIO_PIN_13};
const BspPin_t Dht11Pin = {GPIOA, GPIO_PIN_11};
const BspPin_t WarmPin = {GPIOD, GPIO_PIN_10};
