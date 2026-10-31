#include "app_lcd.h"
#include "app_data.h"

#include "lcd_spi_154.h"   /* SPI_LCD_Init / LCD_DisplayText / LCD_DrawLine_H / Lie / Hang */
#include "app_tick.h"      /* Sem2_LCD（任务节拍信号量） */
#include "cmsis_os.h"

/* ==================== LCD 上要显示的中文 ====================
 * 字库 (Device/Src/lcd_fonts.c) 与驱动 (lcd_spi_154.c) 已统一为 UTF-8：
 * 汉字索引按 UTF-8 三字节存放，驱动按三字节比对，本文件也是 UTF-8，
 * 所以下面可以直接写中文。
 * 注意：只有已收录进 Chinese_2424 字库的字才能显示；要加新字就在 lcd_fonts.c
 *       的 Chinese_2424 表里补「一行点阵 + 一行索引」，索引用 tools/gen_lcd_font.py 生成。 */
#define LCD_TXT_TITLE   "室内温湿度监测"
#define LCD_TXT_TEMP    "温度:"
#define LCD_TXT_HUMI    "湿度:"
#define LCD_TXT_LOW     "下限"
#define LCD_TXT_HIGH    "上限"


/* ==================== 版面：拆成"静态框架"和"数值"两组 ====================*/

/* 静态框架：标题、分隔线、标签、单位，以及数值位的占位符 */
static void Lcd_DrawStatic(void) {
    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LCD_WHITE);

    /* ---- 标题：7 个汉字共 168px，居中 → x = (240-168)/2 = 36 = Lie(3) ---- */
    LCD_DisplayText(Lie(3), Hang(1), LCD_TXT_TITLE);

    /* ---- 分隔线：用中间灰，比文字暗，起分区作用又不抢视线 ---- */
    LCD_SetColor(MID_GREY);
    LCD_DrawLine_H(Lie(1), Hang(2) + 10, 216);   /* 标题下 */
    LCD_DrawLine_H(Lie(1), Hang(5) + 10, 216);   /* 温度区 / 湿度区 之间 */
    LCD_DrawLine_H(Lie(1), Hang(8) + 10, 216);   /* 底部 */
    LCD_SetColor(LCD_WHITE);                /* 恢复画笔色，后面的文字才是白的 */

    /* 数值列取 Lie(8) 是为了湿度可能出现的 3 位数（100%RH）留出余量 */
    LCD_DisplayText(Lie(2), Hang(3), LCD_TXT_TEMP);
    LCD_DisplayText(Lie(11), Hang(3), "℃");
    LCD_DisplayText(Lie(2), Hang(4), LCD_TXT_LOW);
    LCD_DisplayText(Lie(11), Hang(4), LCD_TXT_HIGH);

    LCD_DisplayText(Lie(2), Hang(6), LCD_TXT_HUMI);
    LCD_DisplayText(Lie(11), Hang(6), "%RH");
    LCD_DisplayText(Lie(2), Hang(7), LCD_TXT_LOW);
    LCD_DisplayText(Lie(11), Hang(7), LCD_TXT_HIGH);

    /* ---- 6 个数值位先占位，真数据到了会被 LCD_DisplayNumber 整格覆盖 ---- */
    LCD_DisplayText(Lie(8), Hang(3), "--");    /* 温度实测 */
    LCD_DisplayText(Lie(7), Hang(4), "--");    /* 温度下限 */
    LCD_DisplayText(Lie(16), Hang(4), "--");   /* 温度上限 */
    LCD_DisplayText(Lie(8), Hang(6), "--");    /* 湿度实测 */
    LCD_DisplayText(Lie(7), Hang(7), "--");    /* 湿度下限 */
    LCD_DisplayText(Lie(16), Hang(7), "--");   /* 湿度上限 */
}

/* 数值：只覆盖 6 个数字格。LCD_DisplayNumber(len=2) 用 "%*data" 补空格到固定 2 位 */
static void Lcd_DrawValues(const SensorData_t *data) {
    LCD_SetTextFont(&CH_Font24);
    LCD_SetColor(LCD_WHITE);

    LCD_DisplayNumber(Lie(8), Hang(3), data->Temperature, 2);
    LCD_DisplayNumber(Lie(7), Hang(4), data->Temperature_Alarm[0], 2);
    LCD_DisplayNumber(Lie(16), Hang(4), data->Temperature_Alarm[1], 2);

    LCD_DisplayNumber(Lie(8), Hang(6), data->Humidity, 2);
    LCD_DisplayNumber(Lie(7), Hang(7), data->Humidity_Alarm[0], 2);
    LCD_DisplayNumber(Lie(16), Hang(7), data->Humidity_Alarm[1], 2);
}

void App_Lcd_Run(void) {
    static uint8_t static_drawn = 0;

    for (;;) {
        SensorData_t data;
        uint32_t Flag;

        /* 刷新节奏由节拍服务控制（100 ms） */
        osSemaphoreAcquire(Sem2_LCD, osWaitForever);

        Flag = osEventFlagsWait(EF_SensorData, EVENT_LCD_UPDATE, osFlagsWaitAll, 0U);

        if ((Flag & 0x80000000U) == 0U && (Flag & EVENT_LCD_UPDATE)) {
            /* 加锁拷贝快照 */
            if (osMutexAcquire(Mutex_SensorData, 0U) == osOK) {
                data = SensorData;
                osMutexRelease(Mutex_SensorData);

                if (!static_drawn) {        /* 第一帧真数据：先把框架补上，再填数值 */
                    Lcd_DrawStatic();
                    static_drawn = 1U;
                }
                Lcd_DrawValues(&data);
            }
        } else if (!static_drawn) {
            /* 还没拿到第一帧数据：先把框架和占位符画出来。 */
            Lcd_DrawStatic();
            static_drawn = 1U;
        }
    }
}
