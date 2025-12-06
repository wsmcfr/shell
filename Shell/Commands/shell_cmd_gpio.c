/**
 *******************************************************************************
 * @file    shell_cmd_gpio.c
 * @brief   Xifeng Shell GPIO命令模块
 * @details 此文件实现GPIO相关的Shell命令，包括：
 *          - gpio:   控制GPIO引脚电平
 *          - read:   读取GPIO引脚状态
 *          - led:    控制LED灯
 *          - toggle: 翻转LED状态
 *          - key:    读取按键状态
 *
 * @version 2.0
 * @date    2024
 * @author  Xifeng Shell Team
 *******************************************************************************
 */

#include "shell_core.h"
#include "shell_port.h"
#include "shell_config.h"
#include <stdlib.h>
#include <string.h>

/*===========================================================================*/
/*                              gpio 命令实现                                  */
/*===========================================================================*/

#if SHELL_CMD_GPIO_ENABLE
/**
 * @brief   gpio命令处理函数
 * @details 控制指定GPIO引脚的输出电平
 *
 * @par 使用方法:
 *      gpio <port> <pin> <state>
 *
 * @par 参数说明:
 *      - port:  GPIO端口，A-F
 *      - pin:   引脚号，0-15
 *      - state: 电平状态，0或1
 *
 * @par 示例:
 *      gpio A 5 1    - 设置PA5为高电平
 *      gpio B 0 0    - 设置PB0为低电平
 *
 * @warning 操作前请确认引脚已配置为输出模式
 */
static void shell_cmd_gpio(void)
{
    /* 检查参数数量 */
    if (shell_get_argc() < 4) {
        shell_printf("Usage: gpio <port> <pin> <state>\r\n");
        shell_printf("  gpio A 5 1  - Set PA5 HIGH\r\n");
        shell_printf("  gpio B 0 0  - Set PB0 LOW\r\n");
        return;
    }

    /* 解析端口参数 */
    char port_char = shell_get_argv(1)[0];
    shell_gpio_port_t port;

    /* 将字符转换为端口枚举 */
    if (port_char >= 'A' && port_char <= 'F') {
        port = (shell_gpio_port_t)(port_char - 'A');
    } else if (port_char >= 'a' && port_char <= 'f') {
        port = (shell_gpio_port_t)(port_char - 'a');
    } else {
        shell_printf("Error: Invalid port (A-F)\r\n");
        return;
    }

    /* 解析引脚号 */
    int pin = atoi(shell_get_argv(2));
    if (pin < 0 || pin > 15) {
        shell_printf("Error: Invalid pin (0-15)\r\n");
        return;
    }

    /* 解析状态 */
    int state = atoi(shell_get_argv(3));

    /* 执行GPIO操作 */
    if (shell_port_gpio_write(port, (uint8_t)pin, state ? 1 : 0) == 0) {
        shell_printf("GPIO P%c%d set to %d\r\n",
                     'A' + port, pin, state ? 1 : 0);
    } else {
        shell_printf("Error: Failed to set GPIO\r\n");
    }
}
#endif /* SHELL_CMD_GPIO_ENABLE */

/*===========================================================================*/
/*                              read 命令实现                                  */
/*===========================================================================*/

#if SHELL_CMD_READ_ENABLE
/**
 * @brief   read命令处理函数
 * @details 读取指定GPIO引脚的电平状态
 *
 * @par 使用方法:
 *      read <port> <pin>
 *
 * @par 参数说明:
 *      - port: GPIO端口，A-F
 *      - pin:  引脚号，0-15
 *
 * @par 示例:
 *      read A 5    - 读取PA5状态
 *      read B 0    - 读取PB0状态
 */
static void shell_cmd_read(void)
{
    /* 检查参数数量 */
    if (shell_get_argc() < 3) {
        shell_printf("Usage: read <port> <pin>\r\n");
        shell_printf("  read A 5  - Read PA5 state\r\n");
        shell_printf("  read B 0  - Read PB0 state\r\n");
        return;
    }

    /* 解析端口参数 */
    char port_char = shell_get_argv(1)[0];
    shell_gpio_port_t port;

    if (port_char >= 'A' && port_char <= 'F') {
        port = (shell_gpio_port_t)(port_char - 'A');
    } else if (port_char >= 'a' && port_char <= 'f') {
        port = (shell_gpio_port_t)(port_char - 'a');
    } else {
        shell_printf("Error: Invalid port (A-F)\r\n");
        return;
    }

    /* 解析引脚号 */
    int pin = atoi(shell_get_argv(2));
    if (pin < 0 || pin > 15) {
        shell_printf("Error: Invalid pin (0-15)\r\n");
        return;
    }

    /* 读取GPIO状态 */
    int state = shell_port_gpio_read(port, (uint8_t)pin);
    if (state >= 0) {
        shell_printf("GPIO P%c%d = %d\r\n", 'A' + port, pin, state);
    } else {
        shell_printf("Error: Failed to read GPIO\r\n");
    }
}
#endif /* SHELL_CMD_READ_ENABLE */

/*===========================================================================*/
/*                              led 命令实现                                   */
/*===========================================================================*/

#if SHELL_CMD_LED_ENABLE
/**
 * @brief   led命令处理函数
 * @details 控制LED灯的亮灭状态
 *
 * @par 使用方法:
 *      led <index|all|status> [on|off]
 *
 * @par 参数说明:
 *      - index:  LED索引，0-7
 *      - all:    操作所有LED
 *      - status: 显示LED状态
 *      - on/off: 亮/灭
 *
 * @par 示例:
 *      led 0 on      - 点亮LED0
 *      led 3 off     - 熄灭LED3
 *      led all on    - 点亮所有LED
 *      led all off   - 熄灭所有LED
 *      led status    - 显示所有LED状态
 */
static void shell_cmd_led(void)
{
    int argc = shell_get_argc();

    /* 参数不足时显示帮助 */
    if (argc < 2) {
        shell_printf("Usage: led <index|all|status> [on|off]\r\n");
        shell_printf("  led 0 on     - Turn on LED 0\r\n");
        shell_printf("  led all off  - Turn off all LEDs\r\n");
        shell_printf("  led status   - Show LED status\r\n");
        return;
    }

    char *param1 = shell_get_argv(1);
    uint8_t led_count = shell_port_led_get_count();

    /* 处理status子命令 */
    if (strcmp(param1, "status") == 0) {
        shell_printf("LED Status: ");
        for (uint8_t i = 0; i < led_count; i++) {
            int state = shell_port_led_get(i);
            shell_printf("[%d]%s ", i, state ? "ON" : "OFF");
        }
        shell_printf("\r\n");
        return;
    }

    /* 需要第二个参数（状态） */
    if (argc < 3) {
        shell_printf("Error: Missing state parameter (on/off)\r\n");
        return;
    }

    char *param2 = shell_get_argv(2);

    /* 解析状态 */
    int state = -1;
    if (strcmp(param2, "on") == 0 || strcmp(param2, "1") == 0) {
        state = 1;
    } else if (strcmp(param2, "off") == 0 || strcmp(param2, "0") == 0) {
        state = 0;
    } else {
        shell_printf("Error: Invalid state (use 'on' or 'off')\r\n");
        return;
    }

    /* 处理all子命令 */
    if (strcmp(param1, "all") == 0) {
        for (uint8_t i = 0; i < led_count; i++) {
            shell_port_led_set(i, state);
        }
        shell_printf("All LEDs set to %s\r\n", state ? "ON" : "OFF");
        return;
    }

    /* 处理单个LED */
    int index = atoi(param1);
    if (index < 0 || index >= led_count) {
        shell_printf("Error: Invalid LED index (0-%d)\r\n", led_count - 1);
        return;
    }

    if (shell_port_led_set(index, state) == 0) {
        shell_printf("LED %d set to %s\r\n", index, state ? "ON" : "OFF");
    } else {
        shell_printf("Error: Failed to set LED\r\n");
    }
}
#endif /* SHELL_CMD_LED_ENABLE */

/*===========================================================================*/
/*                              toggle 命令实现                                */
/*===========================================================================*/

#if SHELL_CMD_TOGGLE_ENABLE
/**
 * @brief   toggle命令处理函数
 * @details 翻转LED的状态
 *
 * @par 使用方法:
 *      toggle <index|all>
 *
 * @par 参数说明:
 *      - index: LED索引，0-7
 *      - all:   翻转所有LED
 *
 * @par 示例:
 *      toggle 0     - 翻转LED0
 *      toggle all   - 翻转所有LED
 */
static void shell_cmd_toggle(void)
{
    /* 参数检查 */
    if (shell_get_argc() < 2) {
        shell_printf("Usage: toggle <index|all>\r\n");
        shell_printf("  toggle 0    - Toggle LED 0\r\n");
        shell_printf("  toggle all  - Toggle all LEDs\r\n");
        return;
    }

    char *param = shell_get_argv(1);
    uint8_t led_count = shell_port_led_get_count();

    /* 处理all子命令 */
    if (strcmp(param, "all") == 0) {
        for (uint8_t i = 0; i < led_count; i++) {
            shell_port_led_toggle(i);
        }
        shell_printf("All LEDs toggled\r\n");
        return;
    }

    /* 处理单个LED */
    int index = atoi(param);
    if (index < 0 || index >= led_count) {
        shell_printf("Error: Invalid LED index (0-%d)\r\n", led_count - 1);
        return;
    }

    if (shell_port_led_toggle(index) == 0) {
        int new_state = shell_port_led_get(index);
        shell_printf("LED %d toggled to %s\r\n", index, new_state ? "ON" : "OFF");
    } else {
        shell_printf("Error: Failed to toggle LED\r\n");
    }
}
#endif /* SHELL_CMD_TOGGLE_ENABLE */

/*===========================================================================*/
/*                              key 命令实现                                   */
/*===========================================================================*/

#if SHELL_CMD_KEY_ENABLE
/**
 * @brief   key命令处理函数
 * @details 显示所有按键的当前状态
 *
 * @par 使用方法:
 *      key
 *
 * @par 显示内容:
 *      每个按键的状态（PRESSED或RELEASED）
 */
static void shell_cmd_key(void)
{
    uint8_t key_count = shell_port_key_get_count();

    shell_printf("Key Status:\r\n");
    for (uint8_t i = 0; i < key_count; i++) {
        int state = shell_port_key_read(i);
        shell_printf("  Key%d: %s\r\n", i + 1, state ? "PRESSED" : "RELEASED");
    }
}
#endif /* SHELL_CMD_KEY_ENABLE */

/*===========================================================================*/
/*                              命令注册函数                                   */
/*===========================================================================*/

/**
 * @brief   注册GPIO命令
 * @details 将所有GPIO命令注册到Shell
 *          此函数应在shell_init()之后调用
 */
void shell_cmd_gpio_init(void)
{
#if SHELL_DYNAMIC_CMD_ENABLE
    #if SHELL_CMD_GPIO_ENABLE
    shell_cmd_register("gpio", "GPIO control: gpio <port> <pin> <state>", shell_cmd_gpio);
    #endif

    #if SHELL_CMD_READ_ENABLE
    shell_cmd_register("read", "Read GPIO: read <port> <pin>", shell_cmd_read);
    #endif

    #if SHELL_CMD_LED_ENABLE
    shell_cmd_register("led", "Control LED: led <idx|all|status> [on|off]", shell_cmd_led);
    #endif

    #if SHELL_CMD_TOGGLE_ENABLE
    shell_cmd_register("toggle", "Toggle LED: toggle <idx|all>", shell_cmd_toggle);
    #endif

    #if SHELL_CMD_KEY_ENABLE
    shell_cmd_register("key", "Show key status", shell_cmd_key);
    #endif
#endif
}
