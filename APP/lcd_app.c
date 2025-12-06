/**
 *******************************************************************************
 * @file    lcd_app.c
 * @brief   LCD应用层实现 - 丰富的系统监控界面
 * @details 实现一个美观的LCD显示界面，包含：
 *          - 系统标题栏
 *          - 实时时钟显示
 *          - Shell命令显示
 *          - LED状态可视化
 *          - ADC数据显示
 *          - 系统运行状态
 *
 * @version 2.0
 * @date    2024
 * @author  Xifeng Shell Team
 *
 * @par LCD布局 (320x240, 10行):
 *      Line0: ═══ Xifeng Shell v2.0 ═══  (标题栏-蓝底白字)
 *      Line1: 时间: HH:MM:SS  日期: MM-DD (时间显示)
 *      Line2: ─────────────────────────  (分隔线)
 *      Line3: > [当前Shell命令]          (命令显示-绿色)
 *      Line4: [命令执行结果/输出]         (输出显示)
 *      Line5: ─────────────────────────  (分隔线)
 *      Line6: LED: ■□■□□□□□  ADC0:x.xxV  (LED状态+ADC)
 *      Line7: Freq: xxxxx Hz   ADC1:x.xxV (频率+ADC)
 *      Line8: Var: a=xx b=xx c=xx        (变量显示)
 *      Line9: Uptime: xxd xxh xxm xxs    (运行时间-底部状态栏)
 *******************************************************************************
 */

#include "lcd_app.h"
#include <string.h>
#include <stdio.h>

/*===========================================================================*/
/*                              宏定义                                        */
/*===========================================================================*/

#define LCD_MAX_CMD_LEN     20      /* 命令显示最大长度 */
#define LCD_MAX_OUTPUT_LEN  20      /* 输出显示最大长度 */

/*===========================================================================*/
/*                              静态变量                                       */
/*===========================================================================*/

/** @brief 当前显示的Shell命令 */
static char lcd_shell_cmd[LCD_MAX_CMD_LEN + 1] = "";

/** @brief 当前显示的Shell输出 */
static char lcd_shell_output[LCD_MAX_OUTPUT_LEN + 1] = "";

/** @brief 界面初始化标志 */
static uint8_t lcd_ui_initialized = 0;

/** @brief 闪烁计数器（用于动态效果） */
static uint8_t blink_counter = 0;

/*===========================================================================*/
/*                              内部函数                                       */
/*===========================================================================*/

/**
 * @brief   绘制标题栏
 * @details 蓝色背景，白色文字的标题
 */
static void lcd_draw_title(void)
{
    LCD_SetBackColor(Blue);
    LCD_SetTextColor(White);
    LcdSprintf(Line0, "  Xifeng Shell v2.0 ");
    LCD_SetBackColor(Black);
}

/**
 * @brief   绘制分隔线
 * @param   line: 行号
 */
static void lcd_draw_separator(uint8_t line)
{
    LCD_SetTextColor(Grey);
    LcdSprintf(line, "--------------------");
    LCD_SetTextColor(White);
}

/**
 * @brief   绘制LED状态图标
 * @details 用方块表示LED状态，亮=实心，灭=空心
 */
static void lcd_draw_led_status(void)
{
    char led_str[21];
    char *p = led_str;

    /* 构建LED状态字符串 */
    memcpy(p, "LED:", 4);
    p += 4;

    for (int i = 0; i < 8; i++)
    {
        *p++ = ucLed[i] ? '*' : '.';
    }
    *p = '\0';

    /* 显示LED状态 */
    LCD_SetTextColor(Yellow);
    LCD_DisplayString(Line6, 0, led_str, 12);
}

/**
 * @brief   初始化LCD界面框架
 * @details 绘制静态元素（标题、分隔线等）
 */
static void lcd_init_ui(void)
{
    LCD_Clear(Black);

    /* 绘制标题栏 */
    lcd_draw_title();

    /* 绘制分隔线 */
    lcd_draw_separator(Line2);
    lcd_draw_separator(Line5);

    /* 设置默认颜色 */
    LCD_SetBackColor(Black);
    LCD_SetTextColor(White);

    lcd_ui_initialized = 1;
}

/*===========================================================================*/
/*                              公共函数                                       */
/*===========================================================================*/

/**
 * @brief   格式化字符串并显示到指定的LCD行上
 * @param   Line:   要显示字符串的LCD行号
 * @param   format: 格式化字符串
 */
void LcdSprintf(uint8_t Line, char *format, ...)
{
    char String[21];
    va_list arg;
    va_start(arg, format);
    vsnprintf(String, sizeof(String), format, arg);
    va_end(arg);
    LCD_DisplayStringLine(Line, String);
}

/**
 * @brief   设置LCD显示的Shell命令
 * @param   cmd: 命令字符串
 */
void lcd_set_shell_cmd(const char *cmd)
{
    if (cmd != NULL)
    {
        strncpy(lcd_shell_cmd, cmd, LCD_MAX_CMD_LEN);
        lcd_shell_cmd[LCD_MAX_CMD_LEN] = '\0';
    }
    else
    {
        lcd_shell_cmd[0] = '\0';
    }
}

/**
 * @brief   设置LCD显示的Shell输出
 * @param   output: 输出字符串
 */
void lcd_set_shell_output(const char *output)
{
    if (output != NULL)
    {
        strncpy(lcd_shell_output, output, LCD_MAX_OUTPUT_LEN);
        lcd_shell_output[LCD_MAX_OUTPUT_LEN] = '\0';
    }
    else
    {
        lcd_shell_output[0] = '\0';
    }
}

/**
 * @brief   LCD周期处理函数
 * @details 更新LCD显示内容，由调度器100ms周期调用
 *
 * @par 显示布局:
 *      - 动态更新时间、状态、数据
 *      - 使用不同颜色区分不同信息
 *      - 包含简单动画效果
 */
void lcd_proc(void)
{
    /* 首次运行时初始化界面 */
    if (!lcd_ui_initialized)
    {
        lcd_init_ui();
    }

    /* 闪烁计数器递增 */
    blink_counter++;

    /*========== Line1: 时间和日期显示 ==========*/
    LCD_SetTextColor(Cyan);
    LcdSprintf(Line1, "%02d:%02d:%02d   %02d-%02d-%02d",
               time.Hours, time.Minutes, time.Seconds,
               date.Year, date.Month, date.Date);

    /*========== Line3: Shell命令显示 ==========*/
    LCD_SetTextColor(Green);
    if (lcd_shell_cmd[0] != '\0')
    {
        LcdSprintf(Line3, "> %-18s", lcd_shell_cmd);
    }
    else
    {
        /* 显示闪烁光标效果 */
        if (blink_counter & 0x04)
        {
            LcdSprintf(Line3, "> _                 ");
        }
        else
        {
            LcdSprintf(Line3, ">                   ");
        }
    }

    /*========== Line4: Shell输出显示 ==========*/
    LCD_SetTextColor(White);
    if (lcd_shell_output[0] != '\0')
    {
        LcdSprintf(Line4, "  %-18s", lcd_shell_output);
    }
    else
    {
        LcdSprintf(Line4, "  Ready...          ");
    }

    /*========== Line6: LED状态 + ADC0 ==========*/
    {
        char line_buf[21];
        char led_icons[9];

        /* 构建LED图标 */
        for (int i = 0; i < 8; i++)
        {
            led_icons[i] = ucLed[i] ? '*' : '-';
        }
        led_icons[8] = '\0';

        LCD_SetTextColor(Yellow);
        snprintf(line_buf, sizeof(line_buf), "%s %4.2fV", led_icons, adc_value[0]);
        LCD_DisplayStringLine(Line6, line_buf);
    }

    /*========== Line7: 频率 + ADC1 ==========*/
    LCD_SetTextColor(Magenta);
    LcdSprintf(Line7, "F:%5luHz  A1:%4.2fV", tim_ic_val, adc_value[1]);

    /*========== Line8: 变量显示 ==========*/
    LCD_SetTextColor(White);
    LcdSprintf(Line8, "a:%-4d b:%-4d c:%-4d", vara, varb, varc);

    /*========== Line9: 运行时间状态栏 ==========*/
    {
        uint32_t ticks = HAL_GetTick();
        uint32_t sec = ticks / 1000;
        uint32_t min = sec / 60;
        uint32_t hour = min / 60;
        uint32_t day = hour / 24;

        LCD_SetBackColor(Blue2);
        LCD_SetTextColor(White);

        if (day > 0)
        {
            LcdSprintf(Line9, "Up:%lud%02luh%02lum%02lus  ",
                       day, hour % 24, min % 60, sec % 60);
        }
        else if (hour > 0)
        {
            LcdSprintf(Line9, "Uptime: %02lu:%02lu:%02lu   ",
                       hour, min % 60, sec % 60);
        }
        else
        {
            LcdSprintf(Line9, "Uptime: %02lu:%02lu      ",
                       min, sec % 60);
        }

        LCD_SetBackColor(Black);
    }
}
