/**
 *******************************************************************************
 * @file    shell_port_stm32g4.c
 * @brief   Xifeng Shell STM32G4系列移植层实现
 * @details 此文件实现了Shell移植层接口，适用于STM32G4系列MCU。
 *          如需移植到其他STM32系列，可参考此文件创建对应的移植文件。
 *
 * @version 2.0
 * @date    2024
 * @author  Xifeng Shell Team
 *
 * @note    【移植说明】
 *          1. 此文件依赖STM32 HAL库
 *          2. 需要配置好UART外设（默认使用USART1）
 *          3. 根据实际硬件修改GPIO/LED/按键相关代码
 *
 * @par     硬件配置（CT117E开发板）:
 *          - UART: PA9(TX), PA10(RX), 9600bps
 *          - LED:  PC8-PC15, 低电平点亮
 *          - KEY:  PB0, PB1, PB2, PA0, 按下为低电平
 *******************************************************************************
 */

#include "shell_port.h"
#include "shell_config.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 包含STM32 HAL库头文件 */
#include "main.h"
#include "usart.h"
#include "adc.h"
#include "tim.h"
#include "rtc.h"
#include "gpio.h"

/*===========================================================================*/
/*                              外部变量引用                                   */
/*===========================================================================*/

/**
 * @brief   外部变量声明
 * @details 这些变量在其他模块中定义，用于获取系统状态
 */
extern uint8_t ucLed[8];            /**< LED状态数组 */
extern float adc_value[2];          /**< ADC采样值 */
extern RTC_TimeTypeDef time;        /**< RTC时间 */
extern RTC_DateTypeDef date;        /**< RTC日期 */
extern uint32_t tim_ic_val;         /**< 输入捕获频率值 */

/* 外部函数声明 */
extern void pwm_set_duty(float duty);
extern void pwm_set_frequency(int freq);
extern void eeprom_read(uint8_t *data, uint8_t addr, uint8_t len);
extern void eeprom_write(uint8_t *data, uint8_t addr, uint8_t len);

/*===========================================================================*/
/*                              基础输出函数实现                               */
/*===========================================================================*/

/**
 * @brief   输出单个字符到UART
 * @param   c: 要输出的字符
 *
 * @details 使用HAL库阻塞发送，超时时间10ms
 *          对于高波特率应用，可考虑使用DMA发送
 */
void shell_port_putc(char c)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&c, 1, 10);
}

/**
 * @brief   输出字符串到UART
 * @param   str: 要输出的字符串（以'\0'结尾）
 *
 * @details 计算字符串长度后一次性发送
 *          超时时间根据字符串长度动态计算
 */
void shell_port_puts(const char *str)
{
    uint16_t len = strlen(str);
    /* 超时 = 基础时间 + 每字符时间(约1ms@9600bps) */
    HAL_UART_Transmit(&huart1, (uint8_t *)str, len, 10 + len);
}

/**
 * @brief   格式化输出到UART
 * @param   fmt: 格式化字符串
 * @param   ...: 可变参数
 * @return  输出的字符数
 *
 * @details 使用vsnprintf进行格式化，最大256字节
 *          防止缓冲区溢出
 */
int shell_port_printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    shell_port_puts(buf);
    return len;
}

/*===========================================================================*/
/*                              系统函数实现                                   */
/*===========================================================================*/

/**
 * @brief   获取系统时钟节拍
 * @return  自系统启动以来的毫秒数
 *
 * @details 使用HAL_GetTick()获取，精度1ms
 *          依赖SysTick中断
 */
uint32_t shell_port_get_tick(void)
{
    return HAL_GetTick();
}

/**
 * @brief   毫秒延时
 * @param   ms: 延时毫秒数
 *
 * @details 使用HAL_Delay()实现，阻塞式延时
 *          依赖SysTick中断
 *
 * @warning 在中断中调用会导致死锁
 */
void shell_port_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
 * @brief   系统复位
 *
 * @details 使用NVIC_SystemReset()执行软件复位
 *          复位后程序从头开始执行
 */
void shell_port_reboot(void)
{
    NVIC_SystemReset();
}

/**
 * @brief   获取内存使用信息
 * @param   total: [out] 总内存大小
 * @param   used:  [out] 已使用内存（估算）
 *
 * @details 通过读取栈指针SP估算已使用的栈空间
 *          总内存为配置文件中定义的RAM大小
 *
 * @note    此方法只是估算，不精确
 */
void shell_port_get_mem_info(uint32_t *total, uint32_t *used)
{
    /* 返回配置的RAM大小 */
    *total = SHELL_PLATFORM_RAM_SIZE;

    /* 通过栈指针估算已使用内存 */
    /* 注意：这是ARMCC v5/v6的写法 */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
    register uint32_t sp __asm("sp");
    *used = SHELL_PLATFORM_RAM_END - sp;
#elif defined(__GNUC__)
    register uint32_t sp __asm__("sp");
    *used = SHELL_PLATFORM_RAM_END - sp;
#else
    *used = 0;  /* 不支持的编译器 */
#endif
}

/**
 * @brief   进入临界区（关中断）
 * @return  进入前的中断状态
 *
 * @details 保存PRIMASK并关闭中断
 *          用于保护共享资源
 */
uint32_t shell_port_critical_enter(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

/**
 * @brief   退出临界区（恢复中断）
 * @param   state: 进入临界区时保存的状态
 *
 * @details 恢复PRIMASK到之前的状态
 */
void shell_port_critical_exit(uint32_t state)
{
    __set_PRIMASK(state);
}

/*===========================================================================*/
/*                              GPIO操作实现                                   */
/*===========================================================================*/

/**
 * @brief   GPIO端口映射表
 * @details 将shell_gpio_port_t枚举映射到实际的GPIO_TypeDef指针
 */
static GPIO_TypeDef * const gpio_ports[] = {
    GPIOA,  /* SHELL_GPIO_PORT_A */
    GPIOB,  /* SHELL_GPIO_PORT_B */
    GPIOC,  /* SHELL_GPIO_PORT_C */
    GPIOD,  /* SHELL_GPIO_PORT_D */
    GPIOE,  /* SHELL_GPIO_PORT_E */
    GPIOF,  /* SHELL_GPIO_PORT_F */
#ifdef GPIOG
    GPIOG,  /* SHELL_GPIO_PORT_G */
#else
    NULL,
#endif
};

/**
 * @brief   写入GPIO引脚电平
 * @param   port:  GPIO端口
 * @param   pin:   引脚号 (0-15)
 * @param   state: 电平状态 (0或1)
 * @return  0=成功, -1=失败
 *
 * @details 使用HAL_GPIO_WritePin设置引脚电平
 */
int shell_port_gpio_write(shell_gpio_port_t port, uint8_t pin, uint8_t state)
{
    /* 参数检查 */
    if (port >= SHELL_GPIO_PORT_MAX || pin > 15) {
        return -1;
    }

    GPIO_TypeDef *gpio = gpio_ports[port];
    if (gpio == NULL) {
        return -1;
    }

    /* 设置GPIO电平 */
    HAL_GPIO_WritePin(gpio, (1 << pin), state ? GPIO_PIN_SET : GPIO_PIN_RESET);

    return 0;
}

/**
 * @brief   读取GPIO引脚电平
 * @param   port: GPIO端口
 * @param   pin:  引脚号 (0-15)
 * @return  引脚电平 (0或1), -1=失败
 *
 * @details 使用HAL_GPIO_ReadPin读取引脚电平
 */
int shell_port_gpio_read(shell_gpio_port_t port, uint8_t pin)
{
    /* 参数检查 */
    if (port >= SHELL_GPIO_PORT_MAX || pin > 15) {
        return -1;
    }

    GPIO_TypeDef *gpio = gpio_ports[port];
    if (gpio == NULL) {
        return -1;
    }

    /* 读取GPIO电平 */
    return HAL_GPIO_ReadPin(gpio, (1 << pin));
}

/*===========================================================================*/
/*                              LED操作实现                                    */
/*===========================================================================*/

/**
 * @brief   获取LED数量
 * @return  LED数量（CT117E开发板有8个LED）
 */
uint8_t shell_port_led_get_count(void)
{
    return 8;
}

/**
 * @brief   设置LED状态
 * @param   index: LED索引 (0-7)
 * @param   state: 状态 (0=灭, 1=亮)
 * @return  0=成功, -1=失败
 *
 * @details LED通过锁存器控制，状态保存在ucLed数组中
 *          实际的GPIO操作在led_proc()中完成
 */
int shell_port_led_set(uint8_t index, uint8_t state)
{
    if (index >= 8) {
        return -1;
    }

    ucLed[index] = state ? 1 : 0;
    return 0;
}

/**
 * @brief   获取LED状态
 * @param   index: LED索引 (0-7)
 * @return  LED状态 (0=灭, 1=亮), -1=失败
 */
int shell_port_led_get(uint8_t index)
{
    if (index >= 8) {
        return -1;
    }

    return ucLed[index];
}

/**
 * @brief   翻转LED状态
 * @param   index: LED索引 (0-7)
 * @return  0=成功, -1=失败
 */
int shell_port_led_toggle(uint8_t index)
{
    if (index >= 8) {
        return -1;
    }

    ucLed[index] ^= 1;
    return 0;
}

/*===========================================================================*/
/*                              按键操作实现                                   */
/*===========================================================================*/

/**
 * @brief   获取按键数量
 * @return  按键数量（CT117E开发板有4个按键）
 */
uint8_t shell_port_key_get_count(void)
{
    return 4;
}

/**
 * @brief   读取按键状态
 * @param   index: 按键索引 (0-3)
 * @return  按键状态 (0=释放, 1=按下), -1=失败
 *
 * @details CT117E开发板按键引脚:
 *          - KEY1: PB0
 *          - KEY2: PB1
 *          - KEY3: PB2
 *          - KEY4: PA0
 *          按下时为低电平
 */
int shell_port_key_read(uint8_t index)
{
    GPIO_PinState state;

    switch (index) {
        case 0:
            state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
            break;
        case 1:
            state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
            break;
        case 2:
            state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
            break;
        case 3:
            state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
            break;
        default:
            return -1;
    }

    /* 按下为低电平，返回1表示按下 */
    return (state == GPIO_PIN_RESET) ? 1 : 0;
}

/*===========================================================================*/
/*                              ADC操作实现                                    */
/*===========================================================================*/

/**
 * @brief   读取ADC值
 * @param   channel: ADC通道号 (0 或 1)
 * @return  ADC电压值（伏特），负数表示错误
 *
 * @details adc_value数组由adc_proc()周期性更新
 *          此函数直接返回已处理的电压值
 */
float shell_port_adc_read(uint8_t channel)
{
    if (channel > 1) {
        return -1.0f;
    }

    return adc_value[channel];
}

/*===========================================================================*/
/*                              PWM操作实现                                    */
/*===========================================================================*/

/**
 * @brief   设置PWM占空比
 * @param   channel: PWM通道号（暂时忽略，使用默认通道）
 * @param   duty:    占空比 (0.0 - 100.0)
 * @return  0=成功, -1=失败
 *
 * @details 调用pwm_set_duty()函数设置PWM占空比
 */
int shell_port_pwm_set_duty(uint8_t channel, float duty)
{
    (void)channel;  /* 暂时忽略通道参数 */

    if (duty < 0.0f || duty > 100.0f) {
        return -1;
    }

    pwm_set_duty(duty);
    return 0;
}

/**
 * @brief   设置PWM频率
 * @param   channel: PWM通道号（暂时忽略）
 * @param   freq_hz: 频率（Hz）
 * @return  0=成功, -1=失败
 *
 * @details 调用pwm_set_frequency()函数设置PWM频率
 */
int shell_port_pwm_set_freq(uint8_t channel, uint32_t freq_hz)
{
    (void)channel;  /* 暂时忽略通道参数 */

    if (freq_hz == 0 || freq_hz > 100000) {
        return -1;
    }

    pwm_set_frequency(freq_hz);
    return 0;
}

/*===========================================================================*/
/*                              RTC操作实现                                    */
/*===========================================================================*/

/**
 * @brief   获取RTC时间
 * @param   rtc_time: [out] 时间结构体指针
 * @return  0=成功, -1=失败
 *
 * @details 从全局变量time和date中获取时间
 *          这些变量由rtc_proc()周期性更新
 */
int shell_port_rtc_get(shell_rtc_time_t *rtc_time)
{
    if (rtc_time == NULL) {
        return -1;
    }

    rtc_time->hours = time.Hours;
    rtc_time->minutes = time.Minutes;
    rtc_time->seconds = time.Seconds;
    rtc_time->year = 2000 + date.Year;
    rtc_time->month = date.Month;
    rtc_time->day = date.Date;
    rtc_time->weekday = date.WeekDay;

    return 0;
}

/**
 * @brief   设置RTC时间
 * @param   rtc_time: [in] 时间结构体指针
 * @return  0=成功, -1=失败
 *
 * @details 使用HAL库设置RTC时间和日期
 */
int shell_port_rtc_set(const shell_rtc_time_t *rtc_time)
{
    if (rtc_time == NULL) {
        return -1;
    }

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    /* 设置时间 */
    sTime.Hours = rtc_time->hours;
    sTime.Minutes = rtc_time->minutes;
    sTime.Seconds = rtc_time->seconds;

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        return -1;
    }

    /* 设置日期 */
    sDate.Year = rtc_time->year % 100;  /* RTC只存储00-99 */
    sDate.Month = rtc_time->month;
    sDate.Date = rtc_time->day;
    sDate.WeekDay = rtc_time->weekday;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
        return -1;
    }

    return 0;
}

/*===========================================================================*/
/*                              输入捕获实现                                   */
/*===========================================================================*/

/**
 * @brief   读取输入捕获频率
 * @param   channel: 输入捕获通道号（暂时忽略）
 * @return  频率（Hz），0表示无信号或错误
 *
 * @details tim_ic_val由ic_proc()周期性更新
 */
uint32_t shell_port_ic_get_freq(uint8_t channel)
{
    (void)channel;  /* 暂时忽略通道参数 */
    return tim_ic_val;
}

/*===========================================================================*/
/*                              EEPROM操作实现                                 */
/*===========================================================================*/

/**
 * @brief   读取EEPROM
 * @param   addr: 读取地址
 * @param   data: [out] 数据缓冲区
 * @param   len:  读取长度
 * @return  0=成功, -1=失败
 *
 * @details 调用i2c_hal模块的eeprom_read函数
 */
int shell_port_eeprom_read(uint16_t addr, uint8_t *data, uint16_t len)
{
    if (data == NULL || addr > 255) {
        return -1;
    }

    eeprom_read(data, (uint8_t)addr, (uint8_t)len);
    return 0;
}

/**
 * @brief   写入EEPROM
 * @param   addr: 写入地址
 * @param   data: [in] 数据缓冲区
 * @param   len:  写入长度
 * @return  0=成功, -1=失败
 *
 * @details 调用i2c_hal模块的eeprom_write函数
 */
int shell_port_eeprom_write(uint16_t addr, const uint8_t *data, uint16_t len)
{
    if (data == NULL || addr > 255) {
        return -1;
    }

    /* 注意：eeprom_write的参数不是const，需要强制转换 */
    eeprom_write((uint8_t *)data, (uint8_t)addr, (uint8_t)len);
    return 0;
}

/*===========================================================================*/
/*                              LCD显示实现                                    */
/*===========================================================================*/

/* 引入LCD应用层头文件 */
#include "lcd_app.h"

/**
 * @brief   更新LCD显示的当前命令
 * @param   cmd: 命令字符串
 */
void shell_port_lcd_set_cmd(const char *cmd)
{
    lcd_set_shell_cmd(cmd);
}

/**
 * @brief   更新LCD显示的命令输出
 * @param   output: 输出字符串
 */
void shell_port_lcd_set_output(const char *output)
{
    lcd_set_shell_output(output);
}
