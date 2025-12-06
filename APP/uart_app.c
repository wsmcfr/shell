#include "uart_app.h"
#include "i2c_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUUFER_SIZE 64
#define SHELL_MAX_COMMAND_LENGTH 64
#define SHELL_HISTORY_SIZE 10  // 历史命令最大数量

// 定义环形缓冲区和接收缓冲区
ringbuffer_t usart_rb;
uint8_t usart_read_buffer[BUUFER_SIZE];

// 定义用于Shell命令的缓冲区
char shell_command_buffer[SHELL_MAX_COMMAND_LENGTH];
int shell_command_index = 0;

// 定义命令历史记录
char shell_history[SHELL_HISTORY_SIZE][SHELL_MAX_COMMAND_LENGTH];
int shell_history_index = 0;    // 当前历史记录索引
int shell_history_current = -1; // 上下键使用时的当前选择索引

// ESC序列标志，用于检测上下键等按键序列
int esc_seq_flag = 0;

// 可以通过paraset命令设置的全局变量
int vara = 0;
int varb = 0;
int varc = 0;

// 定义Shell支持的命令结构
typedef void (*shell_command_function_t)(void);

typedef struct
{
    const char *name;
    const char *description;  // 命令描述
    shell_command_function_t function;
} shell_command_t;

// 定义可以自动补全的变量名数组
const char *shell_variables[] =
    {
        "vara",
        "varb",
        "varc",
};

// 命令函数声明
void shell_cmd_help(void);
void shell_cmd_version(void);
void shell_cmd_paraset(void);
void shell_cmd_led(void);
void shell_cmd_adc(void);
void shell_cmd_time(void);
void shell_cmd_pwm(void);
void shell_cmd_reboot(void);
void shell_cmd_gpio(void);
void shell_cmd_status(void);
void shell_cmd_clear(void);
void shell_cmd_echo(void);
void shell_cmd_mem(void);
void shell_cmd_uptime(void);
void shell_cmd_read(void);
void shell_cmd_freq(void);
void shell_cmd_eeprom(void);
void shell_cmd_delay(void);
void shell_cmd_toggle(void);
void shell_cmd_history(void);
void shell_cmd_key(void);
void shell_execute_command(void);
void shell_add_to_history(const char *command);
void shell_tab_complete(void);
void shell_browse_history(int direction);


// 定义支持的命令
shell_command_t shell_commands[] =
    {
        {"help",    "Show all available commands",         shell_cmd_help},
        {"version", "Show shell version",                  shell_cmd_version},
        {"paraset", "Set parameter: paraset <var> <val>",  shell_cmd_paraset},
        {"led",     "Control LED: led <idx|all> <on|off>", shell_cmd_led},
        {"adc",     "Read ADC values",                     shell_cmd_adc},
        {"time",    "Show/set RTC time",                   shell_cmd_time},
        {"pwm",     "Set PWM: pwm <duty> [freq]",          shell_cmd_pwm},
        {"reboot",  "System reboot",                       shell_cmd_reboot},
        {"gpio",    "GPIO control: gpio <port> <pin> <s>", shell_cmd_gpio},
        {"status",  "Show system status",                  shell_cmd_status},
        {"clear",   "Clear screen",                        shell_cmd_clear},
        {"echo",    "Echo text: echo <text>",              shell_cmd_echo},
        {"mem",     "Show memory usage",                   shell_cmd_mem},
        {"uptime",  "Show system uptime",                  shell_cmd_uptime},
        {"read",    "Read GPIO: read <port> <pin>",        shell_cmd_read},
        {"freq",    "Show input capture frequency",        shell_cmd_freq},
        {"eeprom",  "EEPROM: eeprom <r|w> <addr> [data]",  shell_cmd_eeprom},
        {"delay",   "Delay ms: delay <ms>",                shell_cmd_delay},
        {"toggle",  "Toggle LED: toggle <idx|all>",        shell_cmd_toggle},
        {"history", "Show command history",                shell_cmd_history},
        {"key",     "Show key status",                     shell_cmd_key},
};

// UART DMA接收完成回调函数，将接收到的数据写入环形缓冲区
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (!ringbuffer_is_full(&usart_rb))
    {
        ringbuffer_write(&usart_rb, uart_rx_dma_buffer, Size);
    }
    memset(uart_rx_dma_buffer, 0, sizeof(uart_rx_dma_buffer));
}

// 处理UART接收缓冲区中的数据
void uart_proc(void)
{
    if (ringbuffer_is_empty(&usart_rb)) return;

    uint16_t available_data = usart_rb.itemCount;
    ringbuffer_read(&usart_rb, usart_read_buffer, usart_rb.itemCount);

    for (int i = 0; i < available_data; i++)
    {
        char ch = usart_read_buffer[i];

        if (ch == '\r' || ch == '\n') // 回车或换行表示命令输入结束
        {
            shell_command_buffer[shell_command_index] = '\0'; // 添加字符串结束符
            shell_execute_command();                          // 解析并执行命令
            shell_add_to_history(shell_command_buffer);       // 将命令添加到历史记录
            shell_command_index = 0;                          // 重置命令缓冲区索引
            shell_history_current = -1;                       // 重置历史浏览选择
        }
        else if (ch == '\b' && shell_command_index > 0) // 处理退格键
        {
            shell_command_index--;
            printf("\b \b"); // 退格删除屏幕上的字符
        }
        else if (ch == 127 && shell_command_index > 0) // DEL键也作为退格处理
        {
            shell_command_index--;
            printf("\b \b");
        }
        else if (ch == '\t') // 处理TAB自动补全键
        {
            shell_tab_complete();
        }
        else if (ch == 27) // 检测ESC序列开头的控制序列
        {
            esc_seq_flag = 1; // 设置ESC序列标志
        }
        else if (esc_seq_flag == 1 && ch == '[') // 检测到ESC后的[
        {
            esc_seq_flag = 2; // 检测到完整ESC序列的中间部分
        }
        else if (esc_seq_flag == 2) // 处理ESC序列的按键
        {
            if (ch == 'A') // 上键
            {
                shell_browse_history(-1); // 上一条历史命令
            }
            else if (ch == 'B') // 下键
            {
                shell_browse_history(1); // 下一条历史命令
            }
            esc_seq_flag = 0; // 复位ESC标志
        }
        else if (ch >= 32 && ch < 127 && shell_command_index < SHELL_MAX_COMMAND_LENGTH - 1) // 打印可见字符
        {
            shell_command_buffer[shell_command_index++] = ch; // 保存字符到命令缓冲
            printf("%c", ch);                                 // 实时显示输入的字符
        }
    }

    // 清空读取缓冲区
    memset(usart_read_buffer, 0, sizeof(uint8_t) * BUUFER_SIZE);
}

// Shell命令执行函数
void shell_execute_command(void)
{
    printf("\r\n"); // 确保命令执行结果与输入命令的分隔

    // 跳过空命令
    if (shell_command_buffer[0] == '\0')
    {
        printf("> ");
        return;
    }

    for (int i = 0; i < sizeof(shell_commands) / sizeof(shell_command_t); i++)
    {
        if (strncmp(shell_command_buffer, shell_commands[i].name, strlen(shell_commands[i].name)) == 0)
        {
            // 检查命令后是否是空格或结束符（避免前缀匹配问题）
            char next_char = shell_command_buffer[strlen(shell_commands[i].name)];
            if (next_char == '\0' || next_char == ' ')
            {
                shell_commands[i].function(); // 执行对应的命令函数
                printf("\r\n> ");             // 执行后输出打印提示符换行
                return;
            }
        }
    }
    printf("Unknown command: %s\r\n", shell_command_buffer);
    printf("Type 'help' for available commands.\r\n");
    printf("> "); // 打印提示符
}

// 将命令添加到历史记录
void shell_add_to_history(const char *command)
{
    if (command[0] != '\0') // 确保非空命令才添加到历史记录
    {
        strcpy(shell_history[shell_history_index], command);
        shell_history_index = (shell_history_index + 1) % SHELL_HISTORY_SIZE; // 循环存储
    }
}

// TAB补全功能，添加变量自动补全能力
void shell_tab_complete(void)
{
    int match_count = 0;
    const char *match = NULL;
    int cmd_length = 0;

    // 检查当前输入是否是变量
    if (strncmp(shell_command_buffer, "paraset ", 8) == 0)
    {
        cmd_length = 8;
        for (int i = 0; i < sizeof(shell_variables) / sizeof(char *); i++)
        {
            if (strncmp(shell_command_buffer + cmd_length, shell_variables[i], shell_command_index - cmd_length) == 0)
            {
                match = shell_variables[i];
                match_count++;
            }
        }
    }
    else
    {
        for (int i = 0; i < sizeof(shell_commands) / sizeof(shell_command_t); i++)
        {
            if (strncmp(shell_command_buffer, shell_commands[i].name, shell_command_index) == 0)
            {
                match = shell_commands[i].name;
                match_count++;
            }
        }
    }

    // 如果只匹配到一个命令，直接进行补全
    if (match_count == 1 && match != NULL)
    {
        strcpy(shell_command_buffer + cmd_length, match);
        shell_command_index = strlen(shell_command_buffer);
        printf("\r\033[K> %s", shell_command_buffer); // 清除当前行并显示补全后的命令
    }
    else if (match_count > 1) // 如果匹配到多个命令，则显示所有匹配项
    {
        printf("\r\n");      // 换行显示所有匹配的命令列表
        if (cmd_length == 0) // 命令补全
        {
            for (int i = 0; i < sizeof(shell_commands) / sizeof(shell_command_t); i++)
            {
                if (strncmp(shell_command_buffer, shell_commands[i].name, shell_command_index) == 0)
                {
                    printf("%s  ", shell_commands[i].name);
                }
            }
        }
        else // 变量补全
        {
            for (int i = 0; i < sizeof(shell_variables) / sizeof(char *); i++)
            {
                if (strncmp(shell_command_buffer + cmd_length, shell_variables[i], shell_command_index - cmd_length) == 0)
                {
                    printf("%s  ", shell_variables[i]);
                }
            }
        }
        printf("\r\n> %s", shell_command_buffer); // 重新显示提示符和当前输入
    }
}

// 浏览历史命令
void shell_browse_history(int direction)
{
    if (direction == -1 && shell_history_current < shell_history_index - 1)
    {
        shell_history_current++;
    }
    else if (direction == 1 && shell_history_current > 0)
    {
        shell_history_current--;
    }

    printf("\r\033[K> ");
    if (shell_history_current >= 0)
    {
        strcpy(shell_command_buffer, shell_history[shell_history_current]);
        shell_command_index = strlen(shell_command_buffer);
        printf("%s", shell_command_buffer); // 显示历史命令
    }
    else
    {
        shell_command_buffer[0] = '\0'; // 清空命令缓冲
        shell_command_index = 0;
    }
}

//=============================================================================
// Shell命令: help
// 功能: 显示所有可用命令及其描述
//=============================================================================
void shell_cmd_help(void)
{
    printf("Available commands:\r\n");
    printf("----------------------------------------\r\n");
    for (int i = 0; i < sizeof(shell_commands) / sizeof(shell_command_t); i++)
    {
        printf("  %-10s - %s\r\n", shell_commands[i].name, shell_commands[i].description);
    }
    printf("----------------------------------------\r\n");
    printf("Tips: Use TAB for auto-complete, UP/DOWN for history\r\n");
}

//=============================================================================
// Shell命令: version
// 功能: 显示Shell版本信息
//=============================================================================
void shell_cmd_version(void)
{
    printf("==============================\r\n");
    printf("  Xifeng Shell Version 1.1\r\n");
    printf("  Build: %s %s\r\n", __DATE__, __TIME__);
    printf("  MCU: STM32G431RBT6\r\n");
    printf("==============================\r\n");
}

//=============================================================================
// Shell命令: paraset
// 功能: 设置参数值
// 用法: paraset <变量名> <值>
//=============================================================================
void shell_cmd_paraset(void)
{
    char var_name[16];
    int value;

    int parsed = sscanf(shell_command_buffer, "paraset %s %d", var_name, &value);
    if (parsed == 2)
    {
        if (strcmp(var_name, "vara") == 0)
        {
            vara = value;
            printf("vara set to %d\r\n", vara);
        }
        else if (strcmp(var_name, "varb") == 0)
        {
            varb = value;
            printf("varb set to %d\r\n", varb);
        }
        else if (strcmp(var_name, "varc") == 0)
        {
            varc = value;
            printf("varc set to %d\r\n", varc);
        }
        else
        {
            printf("Unknown variable: %s\r\n", var_name);
            printf("Available: vara, varb, varc\r\n");
        }
    }
    else
    {
        printf("Usage: paraset <var_name> <value>\r\n");
        printf("  paraset vara 100  - Set vara to 100\r\n");
    }
}

//=============================================================================
// Shell初始化函数
//=============================================================================
void shell_init(void)
{
    shell_command_index = 0;
//    system_uptime_seconds = 0;
    printf("\r\n");
    printf("======================================\r\n");
    printf("    Xifeng Shell v1.1 Initialized\r\n");
    printf("    Type 'help' for commands\r\n");
    printf("======================================\r\n");
    printf("> "); // 打印提示符
}

//=============================================================================
// Shell命令: led <index> <on/off> 或者 led all <on/off>
// 功能: 控制指定 LED 的状态
// 示例: led 0 on, led 3 off, led all on
//=============================================================================
void shell_cmd_led(void)
{
    char param1[16];
    char param2[16];

    int parsed = sscanf(shell_command_buffer, "led %s %s", param1, param2);

    if (parsed == 2)
    {
        int state = -1;
        if (strcmp(param2, "on") == 0 || strcmp(param2, "1") == 0)
        {
            state = 1;
        }
        else if (strcmp(param2, "off") == 0 || strcmp(param2, "0") == 0)
        {
            state = 0;
        }

        if (state == -1)
        {
            printf("Invalid state. Use 'on' or 'off'\r\n");
            return;
        }

        if (strcmp(param1, "all") == 0)
        {
            // 控制所有的 LED
            for (int i = 0; i < 8; i++)
            {
                ucLed[i] = state;
            }
            printf("All LEDs set to %s\r\n", state ? "ON" : "OFF");
        }
        else
        {
            int index = atoi(param1);
            if (index >= 0 && index < 8)
            {
                ucLed[index] = state;
                printf("LED %d set to %s\r\n", index, state ? "ON" : "OFF");
            }
            else
            {
                printf("Invalid LED index (0-7)\r\n");
            }
        }
    }
    else if (parsed == 1 && strcmp(param1, "status") == 0)
    {
        // 显示LED状态
        printf("LED Status: ");
        for (int i = 0; i < 8; i++)
        {
            printf("[%d]%s ", i, ucLed[i] ? "ON" : "OFF");
        }
        printf("\r\n");
    }
    else
    {
        printf("Usage: led <index|all|status> [on|off]\r\n");
        printf("  led 0 on     - Turn on LED 0\r\n");
        printf("  led all off  - Turn off all LEDs\r\n");
        printf("  led status   - Show LED status\r\n");
    }
}

//=============================================================================
// Shell命令: adc
// 功能: 读取显示 ADC 通道的值
//=============================================================================
void shell_cmd_adc(void)
{
    printf("ADC Values:\r\n");
    printf("  Channel 0: %.3f V\r\n", adc_value[0]);
    printf("  Channel 1: %.3f V\r\n", adc_value[1]);
}

//=============================================================================
// Shell命令: time [set HH:MM:SS] [date YYYY-MM-DD]
// 功能: 显示或设置 RTC 时间/日期
// 示例: time, time set 12:30:00, time date 2024-01-15
//=============================================================================
void shell_cmd_time(void)
{
    char subcmd[16];
    int val1, val2, val3;

    int parsed = sscanf(shell_command_buffer, "time %s %d:%d:%d", subcmd, &val1, &val2, &val3);

    if (parsed == 4 && strcmp(subcmd, "set") == 0)
    {
        // 设置时间
        RTC_TimeTypeDef sTime = {0};
        sTime.Hours = val1;
        sTime.Minutes = val2;
        sTime.Seconds = val3;

        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK)
        {
            printf("Time set to %02d:%02d:%02d\r\n", val1, val2, val3);
        }
        else
        {
            printf("Failed to set time\r\n");
        }
    }
    else
    {
        parsed = sscanf(shell_command_buffer, "time %s %d-%d-%d", subcmd, &val1, &val2, &val3);
        if (parsed == 4 && strcmp(subcmd, "date") == 0)
        {
            // 设置日期
            RTC_DateTypeDef sDate = {0};
            sDate.Year = val1 % 100;  // RTC 只存储 00-99
            sDate.Month = val2;
            sDate.Date = val3;

            if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK)
            {
                printf("Date set to %04d-%02d-%02d\r\n", 2000 + sDate.Year, val2, val3);
            }
            else
            {
                printf("Failed to set date\r\n");
            }
        }
        else
        {
            // 显示当前时间
            printf("Current Time: %02d:%02d:%02d\r\n", time.Hours, time.Minutes, time.Seconds);
            printf("Current Date: 20%02d-%02d-%02d\r\n", date.Year, date.Month, date.Date);
        }
    }
}

//=============================================================================
// Shell命令: pwm <duty> [freq]
// 功能: 设置 PWM 占空比，可选频率
// 示例: pwm 50, pwm 75 1000
//=============================================================================
void shell_cmd_pwm(void)
{
    float duty;
    int freq;

    int parsed = sscanf(shell_command_buffer, "pwm %f %d", &duty, &freq);

    if (parsed >= 1)
    {
        if (duty < 0 || duty > 100)
        {
            printf("Duty cycle must be 0-100%%\r\n");
            return;
        }

        pwm_set_duty(duty);
        printf("PWM duty set to %.1f%%\r\n", duty);

        if (parsed == 2)
        {
            if (freq < 1 || freq > 100000)
            {
                printf("Frequency must be 1-100000 Hz\r\n");
                return;
            }
            pwm_set_frequency(freq);
            printf("PWM frequency set to %d Hz\r\n", freq);
        }
    }
    else
    {
        printf("Usage: pwm <duty> [frequency]\r\n");
        printf("  pwm 50       - Set 50%% duty cycle\r\n");
        printf("  pwm 75 1000  - Set 75%% duty, 1kHz freq\r\n");
    }
}

//=============================================================================
// Shell命令: reboot
// 功能: 系统复位
//=============================================================================
void shell_cmd_reboot(void)
{
    printf("System rebooting...\r\n");
    HAL_Delay(100);  // 等待打印完成
    NVIC_SystemReset();
}

//=============================================================================
// Shell命令: gpio <port> <pin> <state>
// 功能: 控制 GPIO 引脚电平
// 示例: gpio A 5 1, gpio B 0 0
//=============================================================================
void shell_cmd_gpio(void)
{
    char port;
    int pin, state;

    int parsed = sscanf(shell_command_buffer, "gpio %c %d %d", &port, &pin, &state);

    if (parsed == 3)
    {
        GPIO_TypeDef *gpio_port = NULL;

        switch (port)
        {
            case 'A': case 'a': gpio_port = GPIOA; break;
            case 'B': case 'b': gpio_port = GPIOB; break;
            case 'C': case 'c': gpio_port = GPIOC; break;
            case 'D': case 'd': gpio_port = GPIOD; break;
            case 'E': case 'e': gpio_port = GPIOE; break;
            case 'F': case 'f': gpio_port = GPIOF; break;
            default:
                printf("Invalid port (A-F)\r\n");
                return;
        }

        if (pin < 0 || pin > 15)
        {
            printf("Invalid pin (0-15)\r\n");
            return;
        }

        HAL_GPIO_WritePin(gpio_port, (1 << pin), state ? GPIO_PIN_SET : GPIO_PIN_RESET);
        printf("GPIO P%c%d set to %d\r\n", port, pin, state ? 1 : 0);
    }
    else
    {
        printf("Usage: gpio <port> <pin> <state>\r\n");
        printf("  gpio A 5 1  - Set PA5 HIGH\r\n");
        printf("  gpio B 0 0  - Set PB0 LOW\r\n");
    }
}

//=============================================================================
// Shell命令: status
// 功能: 显示系统状态信息
//=============================================================================
void shell_cmd_status(void)
{
    printf("========== System Status ==========\r\n");

    // LED 状态
    printf("LED Status: ");
    for (int i = 0; i < 8; i++)
    {
        printf("%d", ucLed[i]);
    }
    printf("\r\n");

    // ADC 值
    printf("ADC CH0: %.3fV, CH1: %.3fV\r\n", adc_value[0], adc_value[1]);

    // RTC 时间
    printf("Time: %02d:%02d:%02d\r\n", time.Hours, time.Minutes, time.Seconds);
    printf("Date: 20%02d-%02d-%02d\r\n", date.Year, date.Month, date.Date);

    // 输入捕获频率
    printf("Input Capture Freq: %lu Hz\r\n", tim_ic_val);

    // 全局变量
    printf("Variables: vara=%d, varb=%d, varc=%d\r\n", vara, varb, varc);

    printf("===================================\r\n");
}

//=============================================================================
// Shell命令: clear
// 功能: 清屏
//=============================================================================
void shell_cmd_clear(void)
{
    printf("\033[2J\033[H"); // ANSI清屏序列：清除屏幕并将光标移到左上角
}

//=============================================================================
// Shell命令: echo <text>
// 功能: 回显输入的文本
// 示例: echo Hello World
//=============================================================================
void shell_cmd_echo(void)
{
    // 跳过 "echo " 前缀
    if (strlen(shell_command_buffer) > 5)
    {
        printf("%s\r\n", shell_command_buffer + 5);
    }
    else
    {
        printf("\r\n");
    }
}

//=============================================================================
// Shell命令: mem
// 功能: 显示内存使用情况（估算）
//=============================================================================
void shell_cmd_mem(void)
{
    // 获取当前栈指针 (ARMCC兼容写法)
    register uint32_t sp __asm("sp");

    printf("Memory Information:\r\n");
    printf("  RAM Size: 32 KB (0x20000000 - 0x20008000)\r\n");
    printf("  Current SP: 0x%08lX\r\n", sp);
    printf("  Ring Buffer Used: %d/%d bytes\r\n", usart_rb.itemCount, BUUFER_SIZE);
    printf("  History Entries: %d/%d\r\n",
           shell_history_index > 0 ? shell_history_index : 0,
           SHELL_HISTORY_SIZE);
}

//=============================================================================
// Shell命令: uptime
// 功能: 显示系统运行时间
//=============================================================================
void shell_cmd_uptime(void)
{
    uint32_t ticks = HAL_GetTick();
    uint32_t seconds = ticks / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    uint32_t days = hours / 24;

    printf("System Uptime:\r\n");
    printf("  %lu days, %02lu:%02lu:%02lu\r\n",
           days,
           hours % 24,
           minutes % 60,
           seconds % 60);
    printf("  Total ticks: %lu ms\r\n", ticks);
}

//=============================================================================
// Shell命令: read <port> <pin>
// 功能: 读取GPIO引脚电平状态
// 示例: read A 5, read B 0
//=============================================================================
void shell_cmd_read(void)
{
    char port;
    int pin;

    int parsed = sscanf(shell_command_buffer, "read %c %d", &port, &pin);

    if (parsed == 2)
    {
        GPIO_TypeDef *gpio_port = NULL;

        switch (port)
        {
            case 'A': case 'a': gpio_port = GPIOA; break;
            case 'B': case 'b': gpio_port = GPIOB; break;
            case 'C': case 'c': gpio_port = GPIOC; break;
            case 'D': case 'd': gpio_port = GPIOD; break;
            case 'E': case 'e': gpio_port = GPIOE; break;
            case 'F': case 'f': gpio_port = GPIOF; break;
            default:
                printf("Invalid port (A-F)\r\n");
                return;
        }

        if (pin < 0 || pin > 15)
        {
            printf("Invalid pin (0-15)\r\n");
            return;
        }

        GPIO_PinState state = HAL_GPIO_ReadPin(gpio_port, (1 << pin));
        printf("GPIO P%c%d = %d\r\n", port, pin, state);
    }
    else
    {
        printf("Usage: read <port> <pin>\r\n");
        printf("  read A 5  - Read PA5 state\r\n");
        printf("  read B 0  - Read PB0 state\r\n");
    }
}

//=============================================================================
// Shell命令: freq
// 功能: 显示输入捕获测量的频率
//=============================================================================
void shell_cmd_freq(void)
{
    printf("Input Capture Frequency:\r\n");
    printf("  Measured: %lu Hz\r\n", tim_ic_val);
    printf("  Period:   %.2f us\r\n", tim_ic_val > 0 ? 1000000.0f / tim_ic_val : 0);
}

//=============================================================================
// Shell命令: eeprom <r|w> <addr> [data]
// 功能: 读写EEPROM
// 示例: eeprom r 0, eeprom w 0 255
//=============================================================================
void shell_cmd_eeprom(void)
{
    char operation;
    int addr, data;

    int parsed = sscanf(shell_command_buffer, "eeprom %c %d %d", &operation, &addr, &data);

    if (parsed >= 2)
    {
        if (addr < 0 || addr > 255)
        {
            printf("Invalid address (0-255)\r\n");
            return;
        }

        if (operation == 'r' || operation == 'R')
        {
            // 读取EEPROM
            uint8_t read_data;
            eeprom_read(&read_data, (uint8_t)addr, 1);
            printf("EEPROM[0x%02X] = %d (0x%02X)\r\n", addr, read_data, read_data);
        }
        else if ((operation == 'w' || operation == 'W') && parsed == 3)
        {
            // 写入EEPROM
            if (data < 0 || data > 255)
            {
                printf("Invalid data (0-255)\r\n");
                return;
            }
            uint8_t write_data = (uint8_t)data;
            eeprom_write(&write_data, (uint8_t)addr, 1);
            printf("EEPROM[0x%02X] = %d written\r\n", addr, data);
        }
        else
        {
            printf("Usage: eeprom <r|w> <addr> [data]\r\n");
            printf("  eeprom r 0      - Read from address 0\r\n");
            printf("  eeprom w 0 255  - Write 255 to address 0\r\n");
        }
    }
    else
    {
        printf("Usage: eeprom <r|w> <addr> [data]\r\n");
        printf("  eeprom r 0      - Read from address 0\r\n");
        printf("  eeprom w 0 255  - Write 255 to address 0\r\n");
    }
}

//=============================================================================
// Shell命令: delay <ms>
// 功能: 延时指定毫秒数
// 示例: delay 1000
//=============================================================================
void shell_cmd_delay(void)
{
    int ms;

    int parsed = sscanf(shell_command_buffer, "delay %d", &ms);

    if (parsed == 1)
    {
        if (ms < 0 || ms > 60000)
        {
            printf("Delay must be 0-60000 ms\r\n");
            return;
        }
        printf("Delaying %d ms...\r\n", ms);
        HAL_Delay(ms);
        printf("Done.\r\n");
    }
    else
    {
        printf("Usage: delay <ms>\r\n");
        printf("  delay 1000  - Delay 1 second\r\n");
    }
}

//=============================================================================
// Shell命令: toggle <index|all>
// 功能: 翻转LED状态
// 示例: toggle 0, toggle all
//=============================================================================
void shell_cmd_toggle(void)
{
    char param[16];

    int parsed = sscanf(shell_command_buffer, "toggle %s", param);

    if (parsed == 1)
    {
        if (strcmp(param, "all") == 0)
        {
            // 翻转所有LED
            for (int i = 0; i < 8; i++)
            {
                ucLed[i] ^= 1;
            }
            printf("All LEDs toggled\r\n");
        }
        else
        {
            int index = atoi(param);
            if (index >= 0 && index < 8)
            {
                ucLed[index] ^= 1;
                printf("LED %d toggled to %s\r\n", index, ucLed[index] ? "ON" : "OFF");
            }
            else
            {
                printf("Invalid LED index (0-7)\r\n");
            }
        }
    }
    else
    {
        printf("Usage: toggle <index|all>\r\n");
        printf("  toggle 0    - Toggle LED 0\r\n");
        printf("  toggle all  - Toggle all LEDs\r\n");
    }
}

//=============================================================================
// Shell命令: history
// 功能: 显示命令历史记录
//=============================================================================
void shell_cmd_history(void)
{
    printf("Command History:\r\n");
    int count = 0;
    for (int i = 0; i < SHELL_HISTORY_SIZE; i++)
    {
        if (shell_history[i][0] != '\0')
        {
            printf("  %d: %s\r\n", count + 1, shell_history[i]);
            count++;
        }
    }
    if (count == 0)
    {
        printf("  (empty)\r\n");
    }
}

//=============================================================================
// Shell命令: key
// 功能: 显示按键状态
//=============================================================================
void shell_cmd_key(void)
{
    printf("Key Status:\r\n");
    printf("  Key1 (PB0): %s\r\n", HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET ? "PRESSED" : "RELEASED");
    printf("  Key2 (PB1): %s\r\n", HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET ? "PRESSED" : "RELEASED");
    printf("  Key3 (PB2): %s\r\n", HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET ? "PRESSED" : "RELEASED");
    printf("  Key4 (PA0): %s\r\n", HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET ? "PRESSED" : "RELEASED");
}
