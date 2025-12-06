/**
 *******************************************************************************
 * @file    shell_core.h
 * @brief   Xifeng Shell 核心层头文件
 * @details 此文件定义了Shell核心功能的接口，包括：
 *          - Shell初始化和主任务
 *          - 命令注册和管理
 *          - 字符输入处理
 *          - 历史记录和TAB补全
 *
 * @version 2.0
 * @date    2024
 * @author  Xifeng Shell Team
 *
 * @note    此文件是平台无关的，不包含任何硬件相关代码
 *******************************************************************************
 */

#ifndef __SHELL_CORE_H__
#define __SHELL_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "shell_config.h"

/*===========================================================================*/
/*                              类型定义                                       */
/*===========================================================================*/

/**
 * @brief   Shell命令处理函数类型
 * @note    命令函数不带参数，使用shell_get_argc/shell_get_argv获取参数
 */
typedef void (*shell_cmd_func_t)(void);

/**
 * @brief   Shell命令结构体
 * @details 定义一个Shell命令的所有属性
 */
typedef struct {
    const char *name;           /**< 命令名称（字符串） */
    const char *description;    /**< 命令描述（用于help显示） */
    shell_cmd_func_t function;  /**< 命令处理函数指针 */
} shell_cmd_t;

/**
 * @brief   Shell实例状态枚举
 */
typedef enum {
    SHELL_STATE_IDLE = 0,       /**< 空闲状态，等待输入 */
    SHELL_STATE_INPUT,          /**< 输入状态，正在接收字符 */
    SHELL_STATE_EXECUTE,        /**< 执行状态，正在执行命令 */
    SHELL_STATE_ESC_SEQ         /**< ESC序列状态，正在解析控制序列 */
} shell_state_t;

/**
 * @brief   Shell实例结构体
 * @details 包含Shell运行所需的所有状态信息
 */
typedef struct {
    /* 命令行缓冲区 */
    char cmd_buffer[SHELL_CMD_BUFFER_SIZE];     /**< 命令行输入缓冲区 */
    uint16_t cmd_index;                          /**< 当前输入位置索引 */

    /* 参数解析结果 */
    int argc;                                    /**< 参数数量 */
    char *argv[SHELL_MAX_ARGC];                  /**< 参数指针数组 */

#if SHELL_HISTORY_ENABLE
    /* 历史记录 */
    char history[SHELL_HISTORY_SIZE][SHELL_CMD_BUFFER_SIZE];  /**< 历史命令数组 */
    uint8_t history_count;                       /**< 当前历史记录数量 */
    uint8_t history_write_index;                 /**< 历史记录写入索引 */
    int8_t history_browse_index;                 /**< 历史浏览当前索引 */
#endif

    /* 状态机 */
    shell_state_t state;                         /**< 当前状态 */
    uint8_t esc_seq_state;                       /**< ESC序列解析状态 */

    /* 标志位 */
    uint8_t initialized;                         /**< 初始化标志 */
    uint8_t echo_enable;                         /**< 回显开关 */
} shell_t;

/*===========================================================================*/
/*                              全局变量声明                                   */
/*===========================================================================*/

/**
 * @brief   Shell实例（全局唯一）
 * @note    如需多实例，可修改为数组或动态分配
 */
extern shell_t g_shell;

/*===========================================================================*/
/*                              核心API函数                                    */
/*===========================================================================*/

/**
 * @defgroup SHELL_CORE_API Shell核心API
 * @brief    Shell的主要接口函数
 * @{
 */

/**
 * @brief   Shell初始化
 * @details 初始化Shell实例，注册内置命令，显示欢迎信息
 * @note    必须在主循环之前调用一次
 *
 * @par 使用示例:
 * @code
 * int main(void) {
 *     // 硬件初始化...
 *     shell_init();
 *
 *     while(1) {
 *         shell_task();
 *     }
 * }
 * @endcode
 */
void shell_init(void);

/**
 * @brief   Shell主任务
 * @details 在主循环或调度器中周期性调用，处理输入和命令执行
 * @note    推荐调用周期: 10ms
 *
 * @par 使用示例:
 * @code
 * // 方式1: 在主循环中直接调用
 * while(1) {
 *     shell_task();
 *     // 其他任务...
 * }
 *
 * // 方式2: 在任务调度器中调用
 * static task_t tasks[] = {
 *     {shell_task, 10, 0},  // 10ms周期
 *     // 其他任务...
 * };
 * @endcode
 */
void shell_task(void);

/**
 * @brief   向Shell输入单个字符
 * @param   c: 输入的字符
 * @details 此函数处理所有输入字符，包括：
 *          - 可打印字符（添加到命令缓冲区）
 *          - 回车/换行（执行命令）
 *          - 退格/DEL（删除字符）
 *          - TAB（自动补全）
 *          - ESC序列（上下键等）
 *
 * @note    通常由UART接收回调调用
 *
 * @par 使用示例:
 * @code
 * void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
 *     shell_input_char(rx_buffer[0]);
 *     HAL_UART_Receive_IT(&huart1, rx_buffer, 1);
 * }
 * @endcode
 */
void shell_input_char(char c);

/**
 * @brief   向Shell输入字符串
 * @param   str: 输入字符串指针
 * @param   len: 字符串长度
 * @details 用于DMA接收方式，一次性输入多个字符
 *
 * @par 使用示例:
 * @code
 * void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
 *     shell_input_string((char*)dma_buffer, Size);
 * }
 * @endcode
 */
void shell_input_string(const char *str, uint16_t len);

/** @} */ /* End of SHELL_CORE_API */

/*===========================================================================*/
/*                              参数获取函数                                   */
/*===========================================================================*/

/**
 * @defgroup SHELL_ARGS 命令参数获取
 * @brief    用于在命令处理函数中获取参数
 * @{
 */

/**
 * @brief   获取命令参数数量
 * @return  参数数量（包含命令名本身）
 * @note    例如 "led 0 on" 返回3
 *
 * @par 使用示例:
 * @code
 * void shell_cmd_led(void) {
 *     if (shell_get_argc() < 3) {
 *         shell_printf("Usage: led <index> <on|off>\r\n");
 *         return;
 *     }
 *     // 处理参数...
 * }
 * @endcode
 */
int shell_get_argc(void);

/**
 * @brief   获取指定索引的参数
 * @param   index: 参数索引（0=命令名，1=第一个参数...）
 * @return  参数字符串指针，NULL表示索引越界
 *
 * @par 使用示例:
 * @code
 * void shell_cmd_led(void) {
 *     char *cmd = shell_get_argv(0);    // "led"
 *     char *idx = shell_get_argv(1);    // "0"
 *     char *state = shell_get_argv(2);  // "on"
 *
 *     int led_idx = atoi(idx);
 *     // ...
 * }
 * @endcode
 */
char *shell_get_argv(int index);

/**
 * @brief   获取命令行原始字符串
 * @return  完整命令行字符串指针
 * @note    用于需要原始输入的命令（如echo）
 */
const char *shell_get_cmdline(void);

/** @} */ /* End of SHELL_ARGS */

/*===========================================================================*/
/*                              命令注册函数                                   */
/*===========================================================================*/

/**
 * @defgroup SHELL_CMD_REG 命令注册
 * @brief    命令的注册和管理接口
 * @{
 */

/**
 * @brief   注册Shell命令（动态）
 * @param   name:     命令名称
 * @param   desc:     命令描述
 * @param   function: 命令处理函数
 * @return  0=成功, -1=失败（已满或重名）
 *
 * @par 使用示例:
 * @code
 * void my_test_cmd(void) {
 *     shell_printf("Test command executed!\r\n");
 * }
 *
 * void app_init(void) {
 *     shell_cmd_register("test", "Test command", my_test_cmd);
 * }
 * @endcode
 */
#if SHELL_DYNAMIC_CMD_ENABLE
int shell_cmd_register(const char *name, const char *desc, shell_cmd_func_t function);
#endif

/**
 * @brief   注销Shell命令（动态）
 * @param   name: 要注销的命令名称
 * @return  0=成功, -1=失败（未找到）
 */
#if SHELL_DYNAMIC_CMD_ENABLE
int shell_cmd_unregister(const char *name);
#endif

/**
 * @brief   获取已注册的命令数量
 * @return  命令总数（静态+动态）
 */
int shell_get_cmd_count(void);

/**
 * @brief   根据索引获取命令信息
 * @param   index: 命令索引
 * @return  命令结构体指针，NULL表示索引越界
 */
const shell_cmd_t *shell_get_cmd_by_index(int index);

/**
 * @brief   根据名称查找命令
 * @param   name: 命令名称
 * @return  命令结构体指针，NULL表示未找到
 */
const shell_cmd_t *shell_find_cmd(const char *name);

/** @} */ /* End of SHELL_CMD_REG */

/*===========================================================================*/
/*                              输出函数                                       */
/*===========================================================================*/

/**
 * @defgroup SHELL_OUTPUT 输出函数
 * @brief    Shell输出接口（对移植层的封装）
 * @{
 */

/**
 * @brief   Shell格式化输出
 * @param   fmt: 格式化字符串
 * @param   ...: 可变参数
 * @return  输出的字符数
 * @note    封装shell_port_printf，提供统一接口
 */
int shell_printf(const char *fmt, ...);

/**
 * @brief   Shell输出字符串
 * @param   str: 要输出的字符串
 */
void shell_puts(const char *str);

/**
 * @brief   Shell输出单个字符
 * @param   c: 要输出的字符
 */
void shell_putc(char c);

/** @} */ /* End of SHELL_OUTPUT */

/*===========================================================================*/
/*                              历史记录函数                                   */
/*===========================================================================*/

#if SHELL_HISTORY_ENABLE
/**
 * @defgroup SHELL_HISTORY 历史记录
 * @brief    命令历史记录管理
 * @{
 */

/**
 * @brief   添加命令到历史记录
 * @param   cmd: 命令字符串
 * @note    内部使用，自动在命令执行后调用
 */
void shell_history_add(const char *cmd);

/**
 * @brief   浏览历史记录
 * @param   direction: 方向 (-1=上一条, 1=下一条)
 * @note    内部使用，响应上下键
 */
void shell_history_browse(int direction);

/**
 * @brief   获取历史记录数量
 * @return  当前历史记录数量
 */
int shell_history_get_count(void);

/**
 * @brief   获取指定索引的历史记录
 * @param   index: 历史记录索引
 * @return  历史命令字符串，NULL表示索引越界
 */
const char *shell_history_get(int index);

/** @} */ /* End of SHELL_HISTORY */
#endif

/*===========================================================================*/
/*                              TAB补全函数                                    */
/*===========================================================================*/

#if SHELL_TAB_COMPLETE_ENABLE
/**
 * @defgroup SHELL_TAB TAB补全
 * @brief    命令自动补全功能
 * @{
 */

/**
 * @brief   执行TAB补全
 * @note    内部使用，响应TAB键
 */
void shell_tab_complete(void);

/** @} */ /* End of SHELL_TAB */
#endif

/*===========================================================================*/
/*                              命令注册宏                                     */
/*===========================================================================*/

/**
 * @brief   静态命令导出宏
 * @details 使用此宏可以在编译时自动注册命令，无需手动调用注册函数
 *
 * @param   _name:     命令名称（不带引号）
 * @param   _func:     命令处理函数
 * @param   _desc:     命令描述字符串
 *
 * @note    此宏利用编译器的section属性，将命令信息放入特定段
 *          需要在链接脚本中定义相应的段
 *
 * @par 使用示例:
 * @code
 * void shell_cmd_mytest(void) {
 *     shell_printf("My test command!\r\n");
 * }
 * SHELL_CMD_EXPORT(mytest, shell_cmd_mytest, "My test command");
 * @endcode
 *
 * @warning 某些编译器（如ARMCC v5）可能不支持此特性
 *          如不支持，请使用shell_cmd_register()动态注册
 */
#if defined(__CC_ARM) || (defined(__ARMCC_VERSION) && __ARMCC_VERSION >= 6000000)
    /* ARM Compiler 5 / ARM Compiler 6 (armclang) */
    #define SHELL_CMD_EXPORT(_name, _func, _desc)                              \
        const shell_cmd_t __shell_cmd_##_name                                  \
        __attribute__((used, section("shell_cmd"))) = {                        \
            .name = #_name,                                                    \
            .description = _desc,                                              \
            .function = _func                                                  \
        }
#elif defined(__GNUC__)
    /* GCC */
    #define SHELL_CMD_EXPORT(_name, _func, _desc)                              \
        const shell_cmd_t __shell_cmd_##_name                                  \
        __attribute__((used, section(".shell_cmd"))) = {                       \
            .name = #_name,                                                    \
            .description = _desc,                                              \
            .function = _func                                                  \
        }
#elif defined(__ICCARM__)
    /* IAR */
    #define SHELL_CMD_EXPORT(_name, _func, _desc)                              \
        __root const shell_cmd_t __shell_cmd_##_name @ "shell_cmd" = {         \
            .name = #_name,                                                    \
            .description = _desc,                                              \
            .function = _func                                                  \
        }
#else
    /* 不支持的编译器，使用空定义 */
    #warning "SHELL_CMD_EXPORT not supported, use shell_cmd_register() instead"
    #define SHELL_CMD_EXPORT(_name, _func, _desc)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SHELL_CORE_H__ */
