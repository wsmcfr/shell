/**
 *******************************************************************************
 * @file    shell_commands.h
 * @brief   Xifeng Shell 命令模块头文件
 * @details 此文件提供所有命令模块的统一接口，包括：
 *          - 命令模块初始化函数声明
 *          - 命令模块注册的统一入口
 *
 * @version 2.0
 * @date    2024
 * @author  Xifeng Shell Team
 *
 * @note    使用方法：
 *          1. 在shell_init()之后调用shell_commands_init()
 *          2. 所有启用的命令模块将被自动注册
 *******************************************************************************
 */

#ifndef __SHELL_COMMANDS_H__
#define __SHELL_COMMANDS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "shell_config.h"

/*===========================================================================*/
/*                              命令模块初始化函数声明                          */
/*===========================================================================*/

/**
 * @brief   注册系统命令
 * @details 注册reboot, uptime, mem, status, delay等系统命令
 */
void shell_cmd_system_init(void);

/**
 * @brief   注册GPIO命令
 * @details 注册gpio, read, led, toggle, key等GPIO命令
 */
void shell_cmd_gpio_init(void);

/**
 * @brief   注册外设命令
 * @details 注册adc, pwm, time, freq, eeprom, paraset等外设命令
 */
void shell_cmd_peripheral_init(void);

/*===========================================================================*/
/*                              统一初始化函数                                  */
/*===========================================================================*/

/**
 * @brief   初始化所有命令模块
 * @details 调用各命令模块的初始化函数，注册所有启用的命令
 *
 * @par 使用示例:
 * @code
 * int main(void) {
 *     // 硬件初始化...
 *
 *     shell_init();           // 初始化Shell核心
 *     shell_commands_init();  // 注册所有命令
 *
 *     while(1) {
 *         shell_task();
 *     }
 * }
 * @endcode
 *
 * @note    必须在shell_init()之后调用
 */
static inline void shell_commands_init(void)
{
    /* 注册系统命令 */
    shell_cmd_system_init();

    /* 注册GPIO命令 */
    shell_cmd_gpio_init();

    /* 注册外设命令 */
    shell_cmd_peripheral_init();
}

#ifdef __cplusplus
}
#endif

#endif /* __SHELL_COMMANDS_H__ */
