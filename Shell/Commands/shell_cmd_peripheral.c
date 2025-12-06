/**
 *******************************************************************************
 * @file    shell_cmd_peripheral.c
 * @brief   Shell 外设命令模块
 * @details 此文件实现外设相关的Shell命令，包括：
 *          - adc:     读取ADC值
 *          - pwm:     设置PWM输出
 *          - time:    显示/设置RTC时间
 *          - freq:    读取输入捕获频率
 *          - eeprom:  读写EEPROM
 *          - paraset: 设置用户变量
 *
 * @version 2.0
 * @date    2024
 * @author  Shell Team
 *******************************************************************************
 */

#include "shell_core.h"
#include "shell_port.h"
#include "shell_config.h"
#include "persist.h"
#include "tim_app.h"
#include "rtc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*===========================================================================*/
/*                              外部变量引用                                   */
/*===========================================================================*/

/* 用户可设置的全局变量 */
extern int vara, varb, varc;

/* LED状态数组 */
extern uint8_t ucLed[8];

/* RTC */
extern RTC_HandleTypeDef hrtc;

/*===========================================================================*/
/*                              adc 命令实现                                   */
/*===========================================================================*/

#if SHELL_CMD_ADC_ENABLE
/**
 * @brief   adc命令处理函数
 * @details 读取并显示ADC通道的电压值
 *
 * @par 使用方法:
 *      adc [channel]
 *
 * @par 参数说明:
 *      - channel: ADC通道号（可选）。省略则显示所有通道
 *
 * @par 示例:
 *      adc       - 显示所有ADC通道值
 *      adc 0     - 只显示通道0的值
 */
static void shell_cmd_adc(void)
{
    int argc = shell_get_argc();

    if (argc >= 2) {
        /* 指定了通道号 */
        int channel = atoi(shell_get_argv(1));
        float value = shell_port_adc_read(channel);

        if (value >= 0) {
            shell_printf("ADC Channel %d: %.3f V\r\n", channel, value);
        } else {
            shell_printf("Error: Invalid channel\r\n");
        }
    } else {
        /* 显示所有通道 */
        shell_printf("ADC Values:\r\n");
        for (int i = 0; i < 2; i++) {
            float value = shell_port_adc_read(i);
            if (value >= 0) {
                shell_printf("  Channel %d: %.3f V\r\n", i, value);
            }
        }
    }
}
#endif /* SHELL_CMD_ADC_ENABLE */

/*===========================================================================*/
/*                              pwm 命令实现                                   */
/*===========================================================================*/

#if SHELL_CMD_PWM_ENABLE
/**
 * @brief   pwm命令处理函数
 * @details 设置PWM的占空比和频率
 *
 * @par 使用方法:
 *      pwm <duty> [freq]
 *
 * @par 参数说明:
 *      - duty: 占空比，0-100 (%)
 *      - freq: 频率，1-100000 (Hz)，可选
 *
 * @par 示例:
 *      pwm 50        - 设置50%占空比
 *      pwm 75 1000   - 设置75%占空比，1kHz频率
 */
static void shell_cmd_pwm(void)
{
    int argc = shell_get_argc();

    if (argc < 2) {
        shell_printf("Usage: pwm <duty> [freq]\r\n");
        shell_printf("  pwm 50       - Set 50%% duty cycle\r\n");
        shell_printf("  pwm 75 1000  - Set 75%% duty, 1kHz freq\r\n");
        return;
    }

    /* 解析占空比 */
    float duty = (float)atof(shell_get_argv(1));

    /* 范围检查 */
    if (duty < 0 || duty > 100) {
        shell_printf("Error: Duty cycle must be 0-100%%\r\n");
        return;
    }

    /* 设置占空比 */
    if (shell_port_pwm_set_duty(0, duty) == 0) {
        shell_printf("PWM duty set to %.1f%%\r\n", duty);
    } else {
        shell_printf("Error: Failed to set duty\r\n");
        return;
    }

    /* 如果提供了频率参数 */
    if (argc >= 3) {
        int freq = atoi(shell_get_argv(2));

        if (freq < 1 || freq > 100000) {
            shell_printf("Error: Frequency must be 1-100000 Hz\r\n");
            return;
        }

        if (shell_port_pwm_set_freq(0, freq) == 0) {
            shell_printf("PWM frequency set to %d Hz\r\n", freq);
        } else {
            shell_printf("Error: Failed to set frequency\r\n");
        }
    }
}
#endif /* SHELL_CMD_PWM_ENABLE */

/*===========================================================================*/
/*                              time 命令实现                                  */
/*===========================================================================*/

#if SHELL_CMD_TIME_ENABLE
/**
 * @brief   time命令处理函数
 * @details 显示或设置RTC时间和日期
 *
 * @par 使用方法:
 *      time                      - 显示当前时间和日期
 *      time set HH:MM:SS         - 设置时间
 *      time date YYYY-MM-DD      - 设置日期
 *
 * @par 示例:
 *      time                  - 显示当前时间
 *      time set 12:30:00     - 设置时间为12:30:00
 *      time date 2024-01-15  - 设置日期为2024年1月15日
 */
static void shell_cmd_time(void)
{
    int argc = shell_get_argc();
    shell_rtc_time_t rtc_time;

    /* 无参数时显示当前时间 */
    if (argc == 1) {
        if (shell_port_rtc_get(&rtc_time) == 0) {
            shell_printf("Current Time: %02d:%02d:%02d\r\n",
                         rtc_time.hours, rtc_time.minutes, rtc_time.seconds);
            shell_printf("Current Date: %04d-%02d-%02d\r\n",
                         rtc_time.year, rtc_time.month, rtc_time.day);
        } else {
            shell_printf("Error: Failed to read RTC\r\n");
        }
        return;
    }

    /* 有参数时处理设置命令 */
    char *subcmd = shell_get_argv(1);

    if (strcmp(subcmd, "set") == 0 && argc >= 3) {
        /* 设置时间: time set HH:MM:SS */
        int hours, minutes, seconds;

        if (sscanf(shell_get_argv(2), "%d:%d:%d", &hours, &minutes, &seconds) == 3) {
            /* 获取当前日期 */
            shell_port_rtc_get(&rtc_time);

            /* 更新时间 */
            rtc_time.hours = hours;
            rtc_time.minutes = minutes;
            rtc_time.seconds = seconds;

            if (shell_port_rtc_set(&rtc_time) == 0) {
                shell_printf("Time set to %02d:%02d:%02d\r\n", hours, minutes, seconds);
            } else {
                shell_printf("Error: Failed to set time\r\n");
            }
        } else {
            shell_printf("Error: Invalid time format (use HH:MM:SS)\r\n");
        }
    } else if (strcmp(subcmd, "date") == 0 && argc >= 3) {
        /* 设置日期: time date YYYY-MM-DD */
        int year, month, day;

        if (sscanf(shell_get_argv(2), "%d-%d-%d", &year, &month, &day) == 3) {
            /* 获取当前时间 */
            shell_port_rtc_get(&rtc_time);

            /* 更新日期 */
            rtc_time.year = year;
            rtc_time.month = month;
            rtc_time.day = day;

            if (shell_port_rtc_set(&rtc_time) == 0) {
                shell_printf("Date set to %04d-%02d-%02d\r\n", year, month, day);
            } else {
                shell_printf("Error: Failed to set date\r\n");
            }
        } else {
            shell_printf("Error: Invalid date format (use YYYY-MM-DD)\r\n");
        }
    } else {
        shell_printf("Usage:\r\n");
        shell_printf("  time                  - Show current time\r\n");
        shell_printf("  time set HH:MM:SS     - Set time\r\n");
        shell_printf("  time date YYYY-MM-DD  - Set date\r\n");
    }
}
#endif /* SHELL_CMD_TIME_ENABLE */

/*===========================================================================*/
/*                              freq 命令实现                                  */
/*===========================================================================*/

#if SHELL_CMD_FREQ_ENABLE
/**
 * @brief   freq命令处理函数
 * @details 显示输入捕获测量的频率
 *
 * @par 使用方法:
 *      freq
 *
 * @par 显示内容:
 *      - 测量频率 (Hz)
 *      - 信号周期 (us)
 */
static void shell_cmd_freq(void)
{
    uint32_t freq = shell_port_ic_get_freq(0);

    shell_printf("Input Capture Frequency:\r\n");
    shell_printf("  Measured: %lu Hz\r\n", freq);

    if (freq > 0) {
        shell_printf("  Period:   %.2f us\r\n", 1000000.0f / freq);
    } else {
        shell_printf("  Period:   N/A (no signal)\r\n");
    }
}
#endif /* SHELL_CMD_FREQ_ENABLE */

/*===========================================================================*/
/*                              eeprom 命令实现                                */
/*===========================================================================*/

#if SHELL_CMD_EEPROM_ENABLE
/**
 * @brief   eeprom命令处理函数
 * @details 读取或写入EEPROM数据
 *
 * @par 使用方法:
 *      eeprom r <addr>           - 读取指定地址
 *      eeprom w <addr> <data>    - 写入数据到指定地址
 *
 * @par 参数说明:
 *      - addr: 地址，0-255
 *      - data: 数据，0-255
 *
 * @par 示例:
 *      eeprom r 0        - 读取地址0的数据
 *      eeprom w 0 255    - 写入255到地址0
 */
static void shell_cmd_eeprom(void)
{
    int argc = shell_get_argc();

    if (argc < 3) {
        shell_printf("Usage:\r\n");
        shell_printf("  eeprom r <addr>         - Read from address\r\n");
        shell_printf("  eeprom w <addr> <data>  - Write to address\r\n");
        return;
    }

    char operation = shell_get_argv(1)[0];
    int addr = atoi(shell_get_argv(2));

    /* 地址范围检查 */
    if (addr < 0 || addr > 255) {
        shell_printf("Error: Address must be 0-255\r\n");
        return;
    }

    if (operation == 'r' || operation == 'R') {
        /* 读取EEPROM */
        uint8_t data;
        if (shell_port_eeprom_read((uint16_t)addr, &data, 1) == 0) {
            shell_printf("EEPROM[0x%02X] = %d (0x%02X)\r\n", addr, data, data);
        } else {
            shell_printf("Error: Read failed\r\n");
        }
    } else if ((operation == 'w' || operation == 'W') && argc >= 4) {
        /* 写入EEPROM */
        int data = atoi(shell_get_argv(3));

        if (data < 0 || data > 255) {
            shell_printf("Error: Data must be 0-255\r\n");
            return;
        }

        uint8_t write_data = (uint8_t)data;
        if (shell_port_eeprom_write((uint16_t)addr, &write_data, 1) == 0) {
            shell_printf("EEPROM[0x%02X] = %d written\r\n", addr, data);
        } else {
            shell_printf("Error: Write failed\r\n");
        }
    } else {
        shell_printf("Error: Invalid operation\r\n");
    }
}
#endif /* SHELL_CMD_EEPROM_ENABLE */

/*===========================================================================*/
/*                              paraset 命令实现                               */
/*===========================================================================*/

#if SHELL_CMD_PARASET_ENABLE
/**
 * @brief   paraset命令处理函数
 * @details 设置用户自定义变量的值
 *
 * @par 使用方法:
 *      paraset <var> <value>
 *
 * @par 参数说明:
 *      - var:   变量名 (vara, varb, varc)
 *      - value: 要设置的值（整数）
 *
 * @par 示例:
 *      paraset vara 100   - 设置vara为100
 *      paraset varb -50   - 设置varb为-50
 */
static void shell_cmd_paraset(void)
{
    if (shell_get_argc() < 3) {
        shell_printf("Usage: paraset <var_name> <value>\r\n");
        shell_printf("  paraset vara 100  - Set vara to 100\r\n");
        shell_printf("Available variables: vara, varb, varc\r\n");
        shell_printf("Current values: vara=%d, varb=%d, varc=%d\r\n", vara, varb, varc);
        return;
    }

    char *var_name = shell_get_argv(1);
    int value = atoi(shell_get_argv(2));

    if (strcmp(var_name, "vara") == 0) {
        vara = value;
        shell_printf("vara set to %d\r\n", vara);
    } else if (strcmp(var_name, "varb") == 0) {
        varb = value;
        shell_printf("varb set to %d\r\n", varb);
    } else if (strcmp(var_name, "varc") == 0) {
        varc = value;
        shell_printf("varc set to %d\r\n", varc);
    } else {
        shell_printf("Error: Unknown variable '%s'\r\n", var_name);
        shell_printf("Available: vara, varb, varc\r\n");
    }
}
#endif /* SHELL_CMD_PARASET_ENABLE */

/*===========================================================================*/
/*                              save 命令实现                                  */
/*===========================================================================*/

#if SHELL_CMD_SAVE_ENABLE
/**
 * @brief   save命令处理函数
 * @details 保存当前系统数据到EEPROM
 *
 * @par 使用方法:
 *      save
 *
 * @par 保存内容:
 *      - LED状态
 *      - 用户变量 (vara, varb, varc)
 *      - PWM设置 (占空比、频率)
 */
static void shell_cmd_save(void)
{
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

    shell_printf("Saving data to EEPROM...\r\n");

    /* 获取当前RTC时间 */
    HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);

    /* 显示当前数据 */
    shell_printf("  LED State: ");
    for (int i = 0; i < 8; i++) {
        shell_printf("%d", ucLed[i]);
    }
    shell_printf("\r\n");

    shell_printf("  Variables: vara=%d, varb=%d, varc=%d\r\n", vara, varb, varc);
    shell_printf("  PWM: duty=%.1f%%, freq=%dHz\r\n", pwm_get_duty(), pwm_get_frequency());
    shell_printf("  RTC: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
                 rtc_date.Year, rtc_date.Month, rtc_date.Date,
                 rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);

    /* 执行保存 */
    if (persist_save() == 0) {
        shell_printf("Data saved successfully.\r\n");
    } else {
        shell_printf("Error: Save failed!\r\n");
    }
}
#endif /* SHELL_CMD_SAVE_ENABLE */

/*===========================================================================*/
/*                              load 命令实现                                  */
/*===========================================================================*/

#if SHELL_CMD_LOAD_ENABLE
/**
 * @brief   load命令处理函数
 * @details 从EEPROM加载保存的数据
 *
 * @par 使用方法:
 *      load
 */
static void shell_cmd_load(void)
{
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

    shell_printf("Loading data from EEPROM...\r\n");

    if (persist_load() == 0) {
        shell_printf("Data loaded successfully.\r\n");

        /* 获取恢复后的RTC时间 */
        HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);

        /* 显示加载后的数据 */
        shell_printf("  LED State: ");
        for (int i = 0; i < 8; i++) {
            shell_printf("%d", ucLed[i]);
        }
        shell_printf("\r\n");

        shell_printf("  Variables: vara=%d, varb=%d, varc=%d\r\n", vara, varb, varc);
        shell_printf("  PWM: duty=%.1f%%, freq=%dHz\r\n", pwm_get_duty(), pwm_get_frequency());
        shell_printf("  RTC: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
                     rtc_date.Year, rtc_date.Month, rtc_date.Date,
                     rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
    } else {
        shell_printf("Error: No valid data found or CRC mismatch!\r\n");
    }
}
#endif /* SHELL_CMD_LOAD_ENABLE */

/*===========================================================================*/
/*                              factory 命令实现                               */
/*===========================================================================*/

#if SHELL_CMD_FACTORY_ENABLE
/**
 * @brief   factory命令处理函数
 * @details 恢复出厂设置，清除EEPROM中的保存数据
 *
 * @par 使用方法:
 *      factory
 */
static void shell_cmd_factory(void)
{
    shell_printf("Factory reset will clear all saved data.\r\n");
    shell_printf("Resetting to factory defaults...\r\n");

    persist_clear();

    shell_printf("Factory reset complete.\r\n");
    shell_printf("  LED State: 00000000\r\n");
    shell_printf("  Variables: vara=0, varb=0, varc=0\r\n");
    shell_printf("  PWM: duty=50.0%%, freq=1000Hz\r\n");
    shell_printf("  RTC: 2024-01-01 00:00:00\r\n");
}
#endif /* SHELL_CMD_FACTORY_ENABLE */

/*===========================================================================*/
/*                              命令注册函数                                   */
/*===========================================================================*/

/**
 * @brief   注册外设命令
 * @details 将所有外设命令注册到Shell
 *          此函数应在shell_init()之后调用
 */
void shell_cmd_peripheral_init(void)
{
#if SHELL_DYNAMIC_CMD_ENABLE
    #if SHELL_CMD_ADC_ENABLE
    shell_cmd_register("adc", "Read ADC values", shell_cmd_adc);
    #endif

    #if SHELL_CMD_PWM_ENABLE
    shell_cmd_register("pwm", "Set PWM: pwm <duty> [freq]", shell_cmd_pwm);
    #endif

    #if SHELL_CMD_TIME_ENABLE
    shell_cmd_register("time", "Show/set RTC time", shell_cmd_time);
    #endif

    #if SHELL_CMD_FREQ_ENABLE
    shell_cmd_register("freq", "Show input capture frequency", shell_cmd_freq);
    #endif

    #if SHELL_CMD_EEPROM_ENABLE
    shell_cmd_register("eeprom", "EEPROM: eeprom <r|w> <addr> [data]", shell_cmd_eeprom);
    #endif

    #if SHELL_CMD_PARASET_ENABLE
    shell_cmd_register("paraset", "Set parameter: paraset <var> <val>", shell_cmd_paraset);
    #endif

    #if SHELL_CMD_SAVE_ENABLE
    shell_cmd_register("save", "Save data to EEPROM", shell_cmd_save);
    #endif

    #if SHELL_CMD_LOAD_ENABLE
    shell_cmd_register("load", "Load data from EEPROM", shell_cmd_load);
    #endif

    #if SHELL_CMD_FACTORY_ENABLE
    shell_cmd_register("factory", "Factory reset", shell_cmd_factory);
    #endif
#endif
}
