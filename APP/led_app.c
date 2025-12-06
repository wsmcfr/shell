#include "led_app.h"

uint8_t ucLed[8];  // LED 状态数组

/**
 * @brief 显示相关LED
 *
 * @param ucLed Led数据存储数组
 */
void led_disp(uint8_t *ucLed)
{
    // 用于记录当前 LED 状态的临时变量
    uint8_t temp = 0x00;
    // 记录之前 LED 状态的变量，用来判断是否需要更新显示
    static uint8_t temp_old = 0xff;

    for (int i = 0; i < 8; i++)
        // 将当前元素左移相应的位数并累加到temp中
        temp |= (ucLed[i] << (7 - i));

    // 仅当当前状态与之前状态不同的时候，才更新显示
    if (temp != temp_old)
    {
        // 将 GPIOC 高字节清零，低字节更新为新的 temp 值
        GPIOC->ODR &= 0x00ff;       // 清零 GPIOC 高字节
        GPIOC->ODR |= ~(temp << 8); // 设置 GPIOC 高字节为 temp 的反码值
        GPIOD->BSRR |= 0x01 << 2;   // 置位 GPIOD 第2位
        GPIOD->BRR |= 0x01 << 2;    // 复位 GPIOD 第2位
        temp_old = temp;            // 更新记录的旧状态
    }
}

/**
 * @brief LED 显示处理函数
 *
 * 每次调用该函数时，LED 灯根据 ucLed 数组中的值决定是点亮还是关闭
 */
void led_proc(void)
{
    // 显示当前 Led_Pos 位置的 LED 的状态
    led_disp(ucLed);
}
