/**
 *******************************************************************************
 * @file    shell_port.h
 * @brief   Xifeng Shell 移植层接口定义
 * @details 此文件定义了Shell移植层必须实现的所有接口函数。
 *          移植到新平台时，需要实现shell_port_xxx.c中的这些函数。
 *
 * @version 2.0
 * @date    2024
 * @author  Xifeng Shell Team
 *
 * @note    【移植指南】
 *          1. 复制此头文件到目标工程
 *          2. 创建 shell_port_平台名.c 文件
 *          3. 实现本文件中声明的所有函数
 *          4. 根据平台修改 shell_config.h 中的配置
 *******************************************************************************
 */

#ifndef __SHELL_PORT_H__
#define __SHELL_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/*===========================================================================*/
/*                              必须实现的接口                                 */
/*===========================================================================*/

/**
 * @defgroup SHELL_PORT_REQUIRED 必须实现的移植接口
 * @brief    移植时必须实现这些函数，否则Shell无法正常工作
 * @{
 */

/**
 * @brief   输出单个字符到终端
 * @param   c: 要输出的字符
 * @note    此函数是Shell输出的最基本接口，必须实现
 *
 * @par 实现示例 (STM32 HAL):
 * @code
 * void shell_port_putc(char c) {
 *     HAL_UART_Transmit(&huart1, (uint8_t*)&c, 1, 10);
 * }
 * @endcode
 */
void shell_port_putc(char c);

/**
 * @brief   输出字符串到终端
 * @param   str: 要输出的字符串（以'\0'结尾）
 * @note    可以基于shell_port_putc实现，也可以使用DMA提高效率
 *
 * @par 实现示例 (STM32 HAL):
 * @code
 * void shell_port_puts(const char *str) {
 *     HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
 * }
 * @endcode
 */
void shell_port_puts(const char *str);

/**
 * @brief   格式化输出到终端
 * @param   fmt: 格式化字符串
 * @param   ...: 可变参数
 * @return  输出的字符数，负数表示错误
 * @note    类似标准printf函数，建议使用snprintf实现以防止溢出
 *
 * @par 实现示例:
 * @code
 * int shell_port_printf(const char *fmt, ...) {
 *     char buf[256];
 *     va_list args;
 *     va_start(args, fmt);
 *     int len = vsnprintf(buf, sizeof(buf), fmt, args);
 *     va_end(args);
 *     shell_port_puts(buf);
 *     return len;
 * }
 * @endcode
 */
int shell_port_printf(const char *fmt, ...);

/**
 * @brief   获取系统时钟节拍（毫秒）
 * @return  系统启动以来的毫秒数
 * @note    用于计算系统运行时间、超时等
 *
 * @par 实现示例 (STM32 HAL):
 * @code
 * uint32_t shell_port_get_tick(void) {
 *     return HAL_GetTick();
 * }
 * @endcode
 */
uint32_t shell_port_get_tick(void);

/**
 * @brief   毫秒延时函数
 * @param   ms: 延时毫秒数
 * @note    用于delay命令等需要延时的场景
 *
 * @par 实现示例 (STM32 HAL):
 * @code
 * void shell_port_delay_ms(uint32_t ms) {
 *     HAL_Delay(ms);
 * }
 * @endcode
 */
void shell_port_delay_ms(uint32_t ms);

/** @} */ /* End of SHELL_PORT_REQUIRED */

/*===========================================================================*/
/*                              可选实现的接口                                 */
/*===========================================================================*/

/**
 * @defgroup SHELL_PORT_OPTIONAL 可选实现的移植接口
 * @brief    这些函数有默认的弱定义，可根据需要重新实现
 * @{
 */

/**
 * @brief   系统复位
 * @note    用于reboot命令，如不需要可不实现
 *
 * @par 实现示例 (ARM Cortex-M):
 * @code
 * void shell_port_reboot(void) {
 *     NVIC_SystemReset();
 * }
 * @endcode
 */
void shell_port_reboot(void);

/**
 * @brief   获取内存使用信息
 * @param   total: [out] 总内存大小（字节）
 * @param   used:  [out] 已使用内存（字节）
 * @note    用于mem命令，如不需要可不实现
 *
 * @par 实现示例 (获取栈指针估算):
 * @code
 * void shell_port_get_mem_info(uint32_t *total, uint32_t *used) {
 *     *total = 32 * 1024;  // 32KB
 *     register uint32_t sp __asm("sp");
 *     *used = 0x20008000 - sp;  // 估算已使用的栈空间
 * }
 * @endcode
 */
void shell_port_get_mem_info(uint32_t *total, uint32_t *used);

/**
 * @brief   临界区进入（关中断）
 * @return  进入前的中断状态，用于恢复
 * @note    用于保护共享资源，如不需要可不实现
 *
 * @par 实现示例 (ARM Cortex-M):
 * @code
 * uint32_t shell_port_critical_enter(void) {
 *     uint32_t primask = __get_PRIMASK();
 *     __disable_irq();
 *     return primask;
 * }
 * @endcode
 */
uint32_t shell_port_critical_enter(void);

/**
 * @brief   临界区退出（恢复中断）
 * @param   state: 进入临界区时保存的状态
 *
 * @par 实现示例 (ARM Cortex-M):
 * @code
 * void shell_port_critical_exit(uint32_t state) {
 *     __set_PRIMASK(state);
 * }
 * @endcode
 */
void shell_port_critical_exit(uint32_t state);

/** @} */ /* End of SHELL_PORT_OPTIONAL */

/*===========================================================================*/
/*                              GPIO操作接口                                   */
/*===========================================================================*/

/**
 * @defgroup SHELL_PORT_GPIO GPIO操作接口
 * @brief    用于gpio/read/led等命令，根据需要实现
 * @{
 */

/**
 * @brief   GPIO端口枚举
 * @note    根据实际平台定义可用的GPIO端口
 */
typedef enum {
    SHELL_GPIO_PORT_A = 0,
    SHELL_GPIO_PORT_B,
    SHELL_GPIO_PORT_C,
    SHELL_GPIO_PORT_D,
    SHELL_GPIO_PORT_E,
    SHELL_GPIO_PORT_F,
    SHELL_GPIO_PORT_G,
    SHELL_GPIO_PORT_MAX
} shell_gpio_port_t;

/**
 * @brief   写入GPIO引脚电平
 * @param   port:  GPIO端口 (A-G)
 * @param   pin:   引脚号 (0-15)
 * @param   state: 电平状态 (0=低, 1=高)
 * @return  0=成功, -1=失败
 */
int shell_port_gpio_write(shell_gpio_port_t port, uint8_t pin, uint8_t state);

/**
 * @brief   读取GPIO引脚电平
 * @param   port: GPIO端口 (A-G)
 * @param   pin:  引脚号 (0-15)
 * @return  引脚电平 (0或1), -1=失败
 */
int shell_port_gpio_read(shell_gpio_port_t port, uint8_t pin);

/** @} */ /* End of SHELL_PORT_GPIO */

/*===========================================================================*/
/*                              外设操作接口                                   */
/*===========================================================================*/

/**
 * @defgroup SHELL_PORT_PERIPHERAL 外设操作接口
 * @brief    用于adc/pwm/time等外设命令，根据需要实现
 * @{
 */

/**
 * @brief   读取ADC值
 * @param   channel: ADC通道号
 * @return  ADC电压值（伏特），负数表示错误
 */
float shell_port_adc_read(uint8_t channel);

/**
 * @brief   设置PWM占空比
 * @param   channel: PWM通道号
 * @param   duty:    占空比 (0.0 - 100.0)
 * @return  0=成功, -1=失败
 */
int shell_port_pwm_set_duty(uint8_t channel, float duty);

/**
 * @brief   设置PWM频率
 * @param   channel: PWM通道号
 * @param   freq_hz: 频率（Hz）
 * @return  0=成功, -1=失败
 */
int shell_port_pwm_set_freq(uint8_t channel, uint32_t freq_hz);

/**
 * @brief   RTC时间结构体
 */
typedef struct {
    uint8_t hours;      /**< 小时 (0-23) */
    uint8_t minutes;    /**< 分钟 (0-59) */
    uint8_t seconds;    /**< 秒 (0-59) */
    uint16_t year;      /**< 年 (2000-2099) */
    uint8_t month;      /**< 月 (1-12) */
    uint8_t day;        /**< 日 (1-31) */
    uint8_t weekday;    /**< 星期 (0-6, 0=周日) */
} shell_rtc_time_t;

/**
 * @brief   获取RTC时间
 * @param   time: [out] 时间结构体指针
 * @return  0=成功, -1=失败
 */
int shell_port_rtc_get(shell_rtc_time_t *time);

/**
 * @brief   设置RTC时间
 * @param   time: [in] 时间结构体指针
 * @return  0=成功, -1=失败
 */
int shell_port_rtc_set(const shell_rtc_time_t *time);

/**
 * @brief   读取输入捕获频率
 * @param   channel: 输入捕获通道号
 * @return  频率（Hz），0表示无信号或错误
 */
uint32_t shell_port_ic_get_freq(uint8_t channel);

/**
 * @brief   读取EEPROM
 * @param   addr: 读取地址
 * @param   data: [out] 数据缓冲区
 * @param   len:  读取长度
 * @return  0=成功, -1=失败
 */
int shell_port_eeprom_read(uint16_t addr, uint8_t *data, uint16_t len);

/**
 * @brief   写入EEPROM
 * @param   addr: 写入地址
 * @param   data: [in] 数据缓冲区
 * @param   len:  写入长度
 * @return  0=成功, -1=失败
 */
int shell_port_eeprom_write(uint16_t addr, const uint8_t *data, uint16_t len);

/** @} */ /* End of SHELL_PORT_PERIPHERAL */

/*===========================================================================*/
/*                              LED操作接口                                    */
/*===========================================================================*/

/**
 * @defgroup SHELL_PORT_LED LED操作接口
 * @brief    专门用于LED控制的接口
 * @{
 */

/**
 * @brief   获取LED数量
 * @return  系统可用的LED数量
 */
uint8_t shell_port_led_get_count(void);

/**
 * @brief   设置LED状态
 * @param   index: LED索引 (0 开始)
 * @param   state: 状态 (0=灭, 1=亮)
 * @return  0=成功, -1=失败
 */
int shell_port_led_set(uint8_t index, uint8_t state);

/**
 * @brief   获取LED状态
 * @param   index: LED索引 (0 开始)
 * @return  LED状态 (0=灭, 1=亮), -1=失败
 */
int shell_port_led_get(uint8_t index);

/**
 * @brief   翻转LED状态
 * @param   index: LED索引 (0 开始)
 * @return  0=成功, -1=失败
 */
int shell_port_led_toggle(uint8_t index);

/** @} */ /* End of SHELL_PORT_LED */

/*===========================================================================*/
/*                              按键操作接口                                   */
/*===========================================================================*/

/**
 * @defgroup SHELL_PORT_KEY 按键操作接口
 * @brief    用于key命令的接口
 * @{
 */

/**
 * @brief   获取按键数量
 * @return  系统可用的按键数量
 */
uint8_t shell_port_key_get_count(void);

/**
 * @brief   读取按键状态
 * @param   index: 按键索引 (0 开始)
 * @return  按键状态 (0=释放, 1=按下), -1=失败
 */
int shell_port_key_read(uint8_t index);

/** @} */ /* End of SHELL_PORT_KEY */

/*===========================================================================*/
/*                              LCD显示接口（可选）                            */
/*===========================================================================*/

/**
 * @defgroup SHELL_PORT_LCD LCD显示接口
 * @brief    可选的LCD显示更新接口，用于在LCD上显示Shell状态
 * @{
 */

/**
 * @brief   更新LCD显示的当前命令
 * @param   cmd: 命令字符串
 * @note    可选实现，用于在LCD上显示当前执行的命令
 */
void shell_port_lcd_set_cmd(const char *cmd);

/**
 * @brief   更新LCD显示的命令输出
 * @param   output: 输出字符串
 * @note    可选实现，用于在LCD上显示命令执行结果
 */
void shell_port_lcd_set_output(const char *output);

/** @} */ /* End of SHELL_PORT_LCD */

#ifdef __cplusplus
}
#endif

#endif /* __SHELL_PORT_H__ */
