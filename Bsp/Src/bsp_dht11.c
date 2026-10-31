#include "bsp_dht11.h"
#include "bsp_pin.h"

/* 改引脚方向：DHT11 单总线必须在"输出"和"上拉输入"之间来回切。
 * 引脚时钟由 CubeMX 的 MX_GPIO_Init() 统一使能，这里不重复开。 */
static void Dht11_SetMode(uint32_t mode, uint32_t pull) {
    GPIO_InitTypeDef init = {0};

    init.Pin = Dht11Pin.Pin;
    init.Mode = mode;
    init.Pull = pull;
    init.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(Dht11Pin.Port, &init);
}

void Bsp_Dht11_Init(void) {
    Dht11_SetMode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    HAL_GPIO_WritePin(Dht11Pin.Port, Dht11Pin.Pin, GPIO_PIN_SET);   /* 空闲态拉高 */
}

void Bsp_Dht11_SetOutput(void) {
    Dht11_SetMode(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
}

void Bsp_Dht11_SetInputPullup(void) {
    Dht11_SetMode(GPIO_MODE_INPUT, GPIO_PULLUP);   /* 空闲时需要靠上拉保持高电平 */
}

void Bsp_Dht11_Write(uint8_t level) {
    HAL_GPIO_WritePin(Dht11Pin.Port, Dht11Pin.Pin,
                      level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t Bsp_Dht11_Read(void) {
    return (HAL_GPIO_ReadPin(Dht11Pin.Port, Dht11Pin.Pin) == GPIO_PIN_SET) ? 1U : 0U;
}
