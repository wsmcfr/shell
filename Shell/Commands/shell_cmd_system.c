/**
 *******************************************************************************
 * @file    shell_cmd_system.c
 * @brief   Shell 系统命令模块
 * @details 此文件实现系统相关的Shell命令，包括：
 *          - reboot: 系统复位
 *          - uptime: 显示运行时间
 *          - mem:    显示内存使用
 *          - status: 显示系统状态
 *          - delay:  延时指定时间
 *
 * @version 2.0
 * @date    2024
 * @author  Shell Team
 *******************************************************************************
 */

#include "shell_core.h"
#include "shell_port.h"
#include "shell_config.h"
#include <stdlib.h>
#include <string.h>

/* 包含STM32 HAL库头文件（用于RTC类型定义） */
#include "rtc.h"

/*===========================================================================*/
/*                              外部变量引用                                   */
/*===========================================================================*/

/* 用于status命令显示的外部变量 */
extern uint8_t ucLed[8];
extern float adc_value[2];
extern int vara, varb, varc;

/* 外部函数 - 用于获取RTC时间 */
extern RTC_TimeTypeDef time;
extern RTC_DateTypeDef date;
extern uint32_t tim_ic_val;

/*===========================================================================*/
/*                              reboot 命令实现                                */
/*===========================================================================*/

#if SHELL_CMD_REBOOT_ENABLE
/**
 * @brief   reboot命令处理函数
 * @details 执行系统软件复位
 *
 * @par 使用方法:
 *      reboot
 *
 * @note    复位后所有RAM数据会丢失
 */
static void shell_cmd_reboot(void)
{
    shell_printf("System rebooting...\r\n");

    /* 等待输出完成 */
    shell_port_delay_ms(100);

    /* 执行复位 */
    shell_port_reboot();
}

/* 注册命令 - 使用动态注册方式 */
#endif /* SHELL_CMD_REBOOT_ENABLE */

/*===========================================================================*/
/*                              uptime 命令实现                                */
/*===========================================================================*/

#if SHELL_CMD_UPTIME_ENABLE
/**
 * @brief   uptime命令处理函数
 * @details 显示系统自启动以来的运行时间
 *
 * @par 使用方法:
 *      uptime
 *
 * @par 输出格式:
 *      X days, HH:MM:SS
 *      Total ticks: XXXX ms
 */
static void shell_cmd_uptime(void)
{
    /* 获取系统时钟节拍 */
    uint32_t ticks = shell_port_get_tick();

    /* 转换为时分秒 */
    uint32_t seconds = ticks / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    uint32_t days = hours / 24;

    shell_printf("System Uptime:\r\n");
    shell_printf("  %lu days, %02lu:%02lu:%02lu\r\n",
                 days,
                 hours % 24,
                 minutes % 60,
                 seconds % 60);
    shell_printf("  Total ticks: %lu ms\r\n", ticks);
}
#endif /* SHELL_CMD_UPTIME_ENABLE */

/*===========================================================================*/
/*                              mem 命令实现                                   */
/*===========================================================================*/

#if SHELL_CMD_MEM_ENABLE
/**
 * @brief   mem命令处理函数
 * @details 显示系统内存使用情况
 *
 * @par 使用方法:
 *      mem
 *
 * @par 显示内容:
 *      - RAM总大小
 *      - 当前栈指针位置
 *      - 估算的已使用内存
 */
static void shell_cmd_mem(void)
{
    uint32_t total, used;

    /* 获取内存信息 */
    shell_port_get_mem_info(&total, &used);

    shell_printf("Memory Information:\r\n");
    shell_printf("  RAM Size: %lu KB (0x%08lX - 0x%08lX)\r\n",
                 total / 1024,
                 (uint32_t)SHELL_PLATFORM_RAM_START,
                 (uint32_t)SHELL_PLATFORM_RAM_END);
    shell_printf("  Stack Used: ~%lu bytes\r\n", used);

    /* 计算使用百分比 */
    if (total > 0) {
        uint32_t percent = (used * 100) / total;
        shell_printf("  Usage: ~%lu%%\r\n", percent);
    }
}
#endif /* SHELL_CMD_MEM_ENABLE */

/*===========================================================================*/
/*                              status 命令实现                                */
/*===========================================================================*/

#if SHELL_CMD_STATUS_ENABLE
/**
 * @brief   status命令处理函数
 * @details 显示系统综合状态信息
 *
 * @par 使用方法:
 *      status
 *
 * @par 显示内容:
 *      - LED状态
 *      - ADC值
 *      - RTC时间
 *      - 输入捕获频率
 *      - 用户变量
 */
static void shell_cmd_status(void)
{
    shell_printf("========== System Status ==========\r\n");

    /* LED 状态 */
    shell_printf("LED Status: ");
    for (int i = 0; i < 8; i++) {
        shell_printf("%d", ucLed[i]);
    }
    shell_printf("\r\n");

    /* ADC 值 */
    shell_printf("ADC CH0: %.3fV, CH1: %.3fV\r\n", adc_value[0], adc_value[1]);

    /* RTC 时间 */
    shell_printf("Time: %02d:%02d:%02d\r\n", time.Hours, time.Minutes, time.Seconds);
    shell_printf("Date: 20%02d-%02d-%02d\r\n", date.Year, date.Month, date.Date);

    /* 输入捕获频率 */
    shell_printf("Input Capture Freq: %lu Hz\r\n", tim_ic_val);

    /* 全局变量 */
    shell_printf("Variables: vara=%d, varb=%d, varc=%d\r\n", vara, varb, varc);

    shell_printf("===================================\r\n");
}
#endif /* SHELL_CMD_STATUS_ENABLE */

/*===========================================================================*/
/*                              delay 命令实现                                 */
/*===========================================================================*/

#if SHELL_CMD_DELAY_ENABLE
/**
 * @brief   delay命令处理函数
 * @details 阻塞延时指定的毫秒数
 *
 * @par 使用方法:
 *      delay <ms>
 *
 * @par 示例:
 *      delay 1000    - 延时1秒
 *      delay 5000    - 延时5秒
 *
 * @warning 延时期间Shell无法响应其他输入
 */
static void shell_cmd_delay(void)
{
    /* 检查参数数量 */
    if (shell_get_argc() < 2) {
        shell_printf("Usage: delay <ms>\r\n");
        shell_printf("  delay 1000  - Delay 1 second\r\n");
        return;
    }

    /* 解析延时时间 */
    int ms = atoi(shell_get_argv(1));

    /* 范围检查 */
    if (ms < 0 || ms > 60000) {
        shell_printf("Error: Delay must be 0-60000 ms\r\n");
        return;
    }

    shell_printf("Delaying %d ms...\r\n", ms);
    shell_port_delay_ms(ms);
    shell_printf("Done.\r\n");
}
#endif /* SHELL_CMD_DELAY_ENABLE */

/*===========================================================================*/
/*                              命令注册函数                                   */
/*===========================================================================*/

/**
 * @brief   注册系统命令
 * @details 将所有系统命令注册到Shell
 *          此函数应在shell_init()之后调用
 */
void shell_cmd_system_init(void)
{
#if SHELL_DYNAMIC_CMD_ENABLE
    #if SHELL_CMD_REBOOT_ENABLE
    shell_cmd_register("reboot", "System reboot", shell_cmd_reboot);
    #endif

    #if SHELL_CMD_UPTIME_ENABLE
    shell_cmd_register("uptime", "Show system uptime", shell_cmd_uptime);
    #endif

    #if SHELL_CMD_MEM_ENABLE
    shell_cmd_register("mem", "Show memory usage", shell_cmd_mem);
    #endif

    #if SHELL_CMD_STATUS_ENABLE
    shell_cmd_register("status", "Show system status", shell_cmd_status);
    #endif

    #if SHELL_CMD_DELAY_ENABLE
    shell_cmd_register("delay", "Delay ms: delay <ms>", shell_cmd_delay);
    #endif
#endif
}
