#include "system.h"

/**
 * @brief 系统初始化函数
 *
 * 该函数用于初始化系统的 GPIO 配置，包括对 GPIOC 和 GPIOD 输出数据寄存器进行操作。
 * 操作步骤如下：
 * 1. 清零 GPIOC 的 8 位高字节数据寄存器。
 * 2. 将 GPIOC 的 8 位高字节数据寄存器置为高电平。
 * 3. 置位 GPIOD 的第 2 位，然后复位 GPIOD 的第 2 位。
 */
void system_init(void)
{
    // 清零 GPIOC 的 8 位高字节数据寄存器，保留低 8 位
    GPIOC->ODR &= 0x00ff;

    // 将 GPIOC 的 8 位高字节数据寄存器置为高电平
    GPIOC->ODR |= ~(0x00 << 8);

    // 置位 GPIOD 的第 2 位
    GPIOD->BSRR |= 0x01 << 2;

    // 复位 GPIOD 的第 2 位
    GPIOD->BRR  |= 0x01 << 2;
}
