/**
 *******************************************************************************
 * @file    shell_config.h
 * @brief   Shell 配置文件
 * @details 此文件包含Shell的所有可配置选项，用户可以根据实际需求修改这些配置
 *          来裁剪功能或调整参数。
 *
 * @version 2.0
 * @date    2024
 * @author  Shell Team
 *
 * @note    移植到新平台时，只需修改此配置文件和对应的shell_port_xxx.c即可
 *******************************************************************************
 */

#ifndef __SHELL_CONFIG_H__
#define __SHELL_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/*                              版本信息配置                                   */
/*===========================================================================*/

/**
 * @brief Shell版本号
 */
#define SHELL_VERSION_MAJOR     2       /**< 主版本号 */
#define SHELL_VERSION_MINOR     0       /**< 次版本号 */
#define SHELL_VERSION_PATCH     0       /**< 修订号 */

/**
 * @brief Shell名称和构建信息
 */
#define SHELL_NAME              "Shell"
#define SHELL_VERSION_STRING    "v2.0.0"

/*===========================================================================*/
/*                              缓冲区配置                                     */
/*===========================================================================*/

/**
 * @brief 命令行输入缓冲区大小
 * @note  决定了单条命令的最大长度，根据RAM大小调整
 *        推荐值: 64 (最小), 128 (标准), 256 (大型命令)
 */
#define SHELL_CMD_BUFFER_SIZE           128

/**
 * @brief 环形接收缓冲区大小
 * @note  用于UART DMA接收，应大于最大单次传输量
 *        推荐值: 32 (最小), 64 (标准), 128 (高速传输)
 */
#define SHELL_RX_BUFFER_SIZE            64

/**
 * @brief 命令参数最大数量
 * @note  决定了单条命令可以接受的最大参数数目
 *        例如: "led 0 on" 有3个参数 (命令名+2个参数)
 */
#define SHELL_MAX_ARGC                  10

/**
 * @brief 单个参数最大长度
 */
#define SHELL_MAX_ARG_LENGTH            32

/*===========================================================================*/
/*                              历史记录配置                                   */
/*===========================================================================*/

/**
 * @brief 历史记录功能开关
 * @note  设为1启用，设为0禁用以节省RAM
 */
#define SHELL_HISTORY_ENABLE            1

/**
 * @brief 历史记录最大条数
 * @note  每条记录占用 SHELL_CMD_BUFFER_SIZE 字节
 *        总共占用: SHELL_HISTORY_SIZE * SHELL_CMD_BUFFER_SIZE 字节
 *        推荐值: 5 (最小), 10 (标准), 20 (大RAM)
 */
#if SHELL_HISTORY_ENABLE
#define SHELL_HISTORY_SIZE              10
#endif

/*===========================================================================*/
/*                              TAB补全配置                                    */
/*===========================================================================*/

/**
 * @brief TAB自动补全功能开关
 * @note  设为1启用，设为0禁用以节省代码空间
 */
#define SHELL_TAB_COMPLETE_ENABLE       1

/**
 * @brief TAB补全时显示所有匹配项的阈值
 * @note  当匹配项数量超过此值时，询问用户是否显示
 */
#if SHELL_TAB_COMPLETE_ENABLE
#define SHELL_TAB_SHOW_THRESHOLD        20
#endif

/*===========================================================================*/
/*                              提示符配置                                     */
/*===========================================================================*/

/**
 * @brief Shell提示符字符串
 * @note  可自定义提示符，支持颜色转义序列
 *        示例: "> ", "shell> ", "\033[32mshell>\033[0m "
 */
#define SHELL_PROMPT                    "> "

/**
 * @brief 欢迎信息开关
 * @note  设为1在启动时显示欢迎信息，设为0禁用
 */
#define SHELL_BANNER_ENABLE             1

/*===========================================================================*/
/*                              显示配置                                       */
/*===========================================================================*/

/**
 * @brief ANSI颜色/转义序列支持
 * @note  设为1启用彩色输出，设为0用于不支持ANSI的终端
 */
#define SHELL_ANSI_ENABLE               1

/**
 * @brief 回显功能开关
 * @note  设为1回显输入字符，设为0禁用回显
 */
#define SHELL_ECHO_ENABLE               1

/**
 * @brief 换行符配置
 * @note  根据终端类型选择: "\r\n" (Windows), "\n" (Linux/Mac)
 */
#define SHELL_NEWLINE                   "\r\n"

/*===========================================================================*/
/*                              功能模块配置                                   */
/*===========================================================================*/

/**
 * @brief 内置命令模块开关
 * @note  可以单独启用/禁用每个内置命令来裁剪代码
 */
#define SHELL_CMD_HELP_ENABLE           1   /**< help命令 */
#define SHELL_CMD_VERSION_ENABLE        1   /**< version命令 */
#define SHELL_CMD_CLEAR_ENABLE          1   /**< clear命令 */
#define SHELL_CMD_ECHO_ENABLE           1   /**< echo命令 */
#define SHELL_CMD_HISTORY_ENABLE        1   /**< history命令 */

/**
 * @brief 系统命令模块开关
 */
#define SHELL_CMD_REBOOT_ENABLE         1   /**< reboot命令 */
#define SHELL_CMD_UPTIME_ENABLE         1   /**< uptime命令 */
#define SHELL_CMD_MEM_ENABLE            1   /**< mem命令 */
#define SHELL_CMD_STATUS_ENABLE         1   /**< status命令 */
#define SHELL_CMD_DELAY_ENABLE          1   /**< delay命令 */

/**
 * @brief GPIO命令模块开关
 */
#define SHELL_CMD_GPIO_ENABLE           1   /**< gpio命令 */
#define SHELL_CMD_READ_ENABLE           1   /**< read命令 */
#define SHELL_CMD_LED_ENABLE            1   /**< led命令 */
#define SHELL_CMD_TOGGLE_ENABLE         1   /**< toggle命令 */
#define SHELL_CMD_KEY_ENABLE            1   /**< key命令 */

/**
 * @brief 外设命令模块开关
 */
#define SHELL_CMD_ADC_ENABLE            1   /**< adc命令 */
#define SHELL_CMD_PWM_ENABLE            1   /**< pwm命令 */
#define SHELL_CMD_TIME_ENABLE           1   /**< time命令 */
#define SHELL_CMD_FREQ_ENABLE           1   /**< freq命令 */
#define SHELL_CMD_EEPROM_ENABLE         1   /**< eeprom命令 */
#define SHELL_CMD_PARASET_ENABLE        1   /**< paraset命令 */

/*===========================================================================*/
/*                              动态命令注册配置                               */
/*===========================================================================*/

/**
 * @brief 动态命令注册功能开关
 * @note  设为1允许运行时注册新命令，设为0仅使用静态命令
 */
#define SHELL_DYNAMIC_CMD_ENABLE        1

/**
 * @brief 动态命令最大数量
 * @note  仅在SHELL_DYNAMIC_CMD_ENABLE为1时有效
 */
#if SHELL_DYNAMIC_CMD_ENABLE
#define SHELL_DYNAMIC_CMD_MAX           20
#endif

/*===========================================================================*/
/*                              调试配置                                       */
/*===========================================================================*/

/**
 * @brief 调试输出开关
 * @note  设为1输出调试信息，发布时设为0
 */
#define SHELL_DEBUG_ENABLE              0

#if SHELL_DEBUG_ENABLE
#define SHELL_DEBUG(fmt, ...)   shell_printf("[SHELL DBG] " fmt, ##__VA_ARGS__)
#else
#define SHELL_DEBUG(fmt, ...)   ((void)0)
#endif

/*===========================================================================*/
/*                              硬件相关配置                                   */
/*===========================================================================*/

/**
 * @brief 目标平台选择
 * @note  取消注释对应的平台定义
 */
#define SHELL_PLATFORM_STM32G4          /**< STM32G4系列 */
// #define SHELL_PLATFORM_STM32F1       /**< STM32F1系列 */
// #define SHELL_PLATFORM_STM32F4       /**< STM32F4系列 */
// #define SHELL_PLATFORM_STM32H7       /**< STM32H7系列 */

/**
 * @brief RAM大小配置 (字节)
 * @note  用于mem命令显示内存信息
 */
#define SHELL_PLATFORM_RAM_SIZE         (32 * 1024)     /* 32KB for STM32G431 */

/**
 * @brief RAM起始地址
 */
#define SHELL_PLATFORM_RAM_START        0x20000000

/**
 * @brief RAM结束地址
 */
#define SHELL_PLATFORM_RAM_END          (SHELL_PLATFORM_RAM_START + SHELL_PLATFORM_RAM_SIZE)

#ifdef __cplusplus
}
#endif

#endif /* __SHELL_CONFIG_H__ */
