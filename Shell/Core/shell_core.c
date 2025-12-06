/**
 *******************************************************************************
 * @file    shell_core.c
 * @brief   Shell 核心实现
 * @details 此文件实现了Shell的核心功能，包括：
 *          - 命令行解析和执行
 *          - TAB自动补全
 *          - 历史记录管理
 *          - ESC序列处理（上下键等）
 *
 * @version 2.0
 * @date    2024
 * @author  Shell Team
 *
 * @note    此文件是平台无关的，所有硬件操作通过shell_port.h中的接口完成
 *******************************************************************************
 */

#include "shell_core.h"
#include "shell_port.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/*===========================================================================*/
/*                              全局变量定义                                   */
/*===========================================================================*/

/**
 * @brief   Shell全局实例
 * @details 存储Shell的所有状态信息，支持单实例模式
 */
shell_t g_shell;

/*===========================================================================*/
/*                              内置命令声明                                   */
/*===========================================================================*/

/* 前向声明内置命令函数 */
#if SHELL_CMD_HELP_ENABLE
static void shell_cmd_help(void);
#endif

#if SHELL_CMD_VERSION_ENABLE
static void shell_cmd_version(void);
#endif

#if SHELL_CMD_CLEAR_ENABLE
static void shell_cmd_clear(void);
#endif

#if SHELL_CMD_ECHO_ENABLE
static void shell_cmd_echo(void);
#endif

#if SHELL_HISTORY_ENABLE && SHELL_CMD_HISTORY_ENABLE
static void shell_cmd_history(void);
#endif

/*===========================================================================*/
/*                              静态命令表                                     */
/*===========================================================================*/

/**
 * @brief   内置命令表（静态定义）
 * @details 编译时确定的命令列表，存储在Flash中节省RAM
 *          用户可以通过shell_config.h中的宏来启用/禁用特定命令
 */
static const shell_cmd_t shell_builtin_cmds[] = {
#if SHELL_CMD_HELP_ENABLE
    {"help",    "Show all available commands",  shell_cmd_help},
#endif
#if SHELL_CMD_VERSION_ENABLE
    {"version", "Show shell version",           shell_cmd_version},
#endif
#if SHELL_CMD_CLEAR_ENABLE
    {"clear",   "Clear screen",                 shell_cmd_clear},
#endif
#if SHELL_CMD_ECHO_ENABLE
    {"echo",    "Echo text: echo <text>",       shell_cmd_echo},
#endif
#if SHELL_HISTORY_ENABLE && SHELL_CMD_HISTORY_ENABLE
    {"history", "Show command history",         shell_cmd_history},
#endif
};

/**
 * @brief   内置命令数量
 */
#define SHELL_BUILTIN_CMD_COUNT (sizeof(shell_builtin_cmds) / sizeof(shell_cmd_t))

/*===========================================================================*/
/*                              动态命令表                                     */
/*===========================================================================*/

#if SHELL_DYNAMIC_CMD_ENABLE
/**
 * @brief   动态命令表
 * @details 运行时注册的命令存储在此数组中
 */
static shell_cmd_t shell_dynamic_cmds[SHELL_DYNAMIC_CMD_MAX];

/**
 * @brief   当前动态命令数量
 */
static int shell_dynamic_cmd_count = 0;
#endif

/*===========================================================================*/
/*                              内部函数声明                                   */
/*===========================================================================*/

static void shell_parse_args(void);
static void shell_execute(void);
static void shell_show_prompt(void);
static void shell_clear_line(void);
static void shell_process_char(char c);
static void shell_process_esc_seq(char c);

/*===========================================================================*/
/*                              输出函数实现                                   */
/*===========================================================================*/

/**
 * @brief   Shell格式化输出
 * @param   fmt: 格式化字符串
 * @param   ...: 可变参数
 * @return  输出的字符数
 *
 * @details 使用vsnprintf进行格式化，防止缓冲区溢出
 *          最大输出长度为256字节，超出部分会被截断
 */
int shell_printf(const char *fmt, ...)
{
    char buf[256];          /* 格式化缓冲区 */
    va_list args;

    /* 使用可变参数进行格式化 */
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* 输出格式化后的字符串 */
    shell_port_puts(buf);

    return len;
}

/**
 * @brief   Shell输出字符串
 * @param   str: 要输出的字符串
 */
void shell_puts(const char *str)
{
    shell_port_puts(str);
}

/**
 * @brief   Shell输出单个字符
 * @param   c: 要输出的字符
 */
void shell_putc(char c)
{
    shell_port_putc(c);
}

/*===========================================================================*/
/*                              命令注册函数实现                               */
/*===========================================================================*/

#if SHELL_DYNAMIC_CMD_ENABLE
/**
 * @brief   注册动态命令
 * @param   name:     命令名称
 * @param   desc:     命令描述
 * @param   function: 命令处理函数
 * @return  0=成功, -1=失败
 *
 * @details 检查重名和数组满的情况
 *          命令信息存储在shell_dynamic_cmds数组中
 */
int shell_cmd_register(const char *name, const char *desc, shell_cmd_func_t function)
{
    /* 参数检查 */
    if (name == NULL || function == NULL) {
        return -1;
    }

    /* 检查是否已满 */
    if (shell_dynamic_cmd_count >= SHELL_DYNAMIC_CMD_MAX) {
        shell_printf("Error: Dynamic command table full\r\n");
        return -1;
    }

    /* 检查是否已存在同名命令 */
    if (shell_find_cmd(name) != NULL) {
        shell_printf("Error: Command '%s' already exists\r\n", name);
        return -1;
    }

    /* 添加到动态命令表 */
    shell_dynamic_cmds[shell_dynamic_cmd_count].name = name;
    shell_dynamic_cmds[shell_dynamic_cmd_count].description = desc;
    shell_dynamic_cmds[shell_dynamic_cmd_count].function = function;
    shell_dynamic_cmd_count++;

    return 0;
}

/**
 * @brief   注销动态命令
 * @param   name: 命令名称
 * @return  0=成功, -1=未找到
 *
 * @details 从动态命令表中移除指定命令
 *          后续命令前移填补空位
 */
int shell_cmd_unregister(const char *name)
{
    /* 在动态命令表中查找 */
    for (int i = 0; i < shell_dynamic_cmd_count; i++) {
        if (strcmp(shell_dynamic_cmds[i].name, name) == 0) {
            /* 将后面的命令前移 */
            for (int j = i; j < shell_dynamic_cmd_count - 1; j++) {
                shell_dynamic_cmds[j] = shell_dynamic_cmds[j + 1];
            }
            shell_dynamic_cmd_count--;
            return 0;
        }
    }

    return -1;  /* 未找到 */
}
#endif /* SHELL_DYNAMIC_CMD_ENABLE */

/**
 * @brief   获取已注册命令总数
 * @return  命令总数（内置+动态）
 */
int shell_get_cmd_count(void)
{
#if SHELL_DYNAMIC_CMD_ENABLE
    return SHELL_BUILTIN_CMD_COUNT + shell_dynamic_cmd_count;
#else
    return SHELL_BUILTIN_CMD_COUNT;
#endif
}

/**
 * @brief   根据索引获取命令
 * @param   index: 命令索引
 * @return  命令结构体指针
 *
 * @details 先搜索内置命令，再搜索动态命令
 */
const shell_cmd_t *shell_get_cmd_by_index(int index)
{
    /* 检查索引范围 */
    if (index < 0) {
        return NULL;
    }

    /* 在内置命令中查找 */
    if (index < (int)SHELL_BUILTIN_CMD_COUNT) {
        return &shell_builtin_cmds[index];
    }

#if SHELL_DYNAMIC_CMD_ENABLE
    /* 在动态命令中查找 */
    int dynamic_index = index - SHELL_BUILTIN_CMD_COUNT;
    if (dynamic_index < shell_dynamic_cmd_count) {
        return &shell_dynamic_cmds[dynamic_index];
    }
#endif

    return NULL;
}

/**
 * @brief   根据名称查找命令
 * @param   name: 命令名称
 * @return  命令结构体指针，未找到返回NULL
 *
 * @details 遍历所有命令（内置+动态）进行字符串比较
 */
const shell_cmd_t *shell_find_cmd(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    /* 在内置命令中查找 */
    for (int i = 0; i < (int)SHELL_BUILTIN_CMD_COUNT; i++) {
        if (strcmp(shell_builtin_cmds[i].name, name) == 0) {
            return &shell_builtin_cmds[i];
        }
    }

#if SHELL_DYNAMIC_CMD_ENABLE
    /* 在动态命令中查找 */
    for (int i = 0; i < shell_dynamic_cmd_count; i++) {
        if (strcmp(shell_dynamic_cmds[i].name, name) == 0) {
            return &shell_dynamic_cmds[i];
        }
    }
#endif

    return NULL;
}

/*===========================================================================*/
/*                              参数获取函数实现                               */
/*===========================================================================*/

/**
 * @brief   获取参数数量
 * @return  参数数量（包含命令名）
 */
int shell_get_argc(void)
{
    return g_shell.argc;
}

/**
 * @brief   获取指定索引的参数
 * @param   index: 参数索引
 * @return  参数字符串指针
 */
char *shell_get_argv(int index)
{
    if (index < 0 || index >= g_shell.argc) {
        return NULL;
    }
    return g_shell.argv[index];
}

/**
 * @brief   获取原始命令行
 * @return  命令行字符串指针
 */
const char *shell_get_cmdline(void)
{
    return g_shell.cmd_buffer;
}

/*===========================================================================*/
/*                              历史记录函数实现                               */
/*===========================================================================*/

#if SHELL_HISTORY_ENABLE
/**
 * @brief   添加命令到历史记录
 * @param   cmd: 命令字符串
 *
 * @details 使用循环缓冲区存储历史命令
 *          空命令和与上一条相同的命令不会被添加
 */
void shell_history_add(const char *cmd)
{
    /* 检查空命令 */
    if (cmd == NULL || cmd[0] == '\0') {
        return;
    }

    /* 检查是否与最后一条命令相同 */
    if (g_shell.history_count > 0) {
        int last_idx = (g_shell.history_write_index - 1 + SHELL_HISTORY_SIZE)
                       % SHELL_HISTORY_SIZE;
        if (strcmp(g_shell.history[last_idx], cmd) == 0) {
            return;  /* 跳过重复命令 */
        }
    }

    /* 添加到历史记录 */
    strncpy(g_shell.history[g_shell.history_write_index],
            cmd,
            SHELL_CMD_BUFFER_SIZE - 1);
    g_shell.history[g_shell.history_write_index][SHELL_CMD_BUFFER_SIZE - 1] = '\0';

    /* 更新写入索引（循环） */
    g_shell.history_write_index = (g_shell.history_write_index + 1) % SHELL_HISTORY_SIZE;

    /* 更新记录数量 */
    if (g_shell.history_count < SHELL_HISTORY_SIZE) {
        g_shell.history_count++;
    }
}

/**
 * @brief   浏览历史记录
 * @param   direction: -1=上一条（更旧），1=下一条（更新）
 *
 * @details 响应上下键，在历史记录中导航
 *          更新命令缓冲区并显示到终端
 */
void shell_history_browse(int direction)
{
    if (g_shell.history_count == 0) {
        return;  /* 没有历史记录 */
    }

    /* 计算新的浏览索引 */
    if (direction == -1) {  /* 上键，查看更旧的命令 */
        if (g_shell.history_browse_index < (int)g_shell.history_count - 1) {
            g_shell.history_browse_index++;
        }
    } else if (direction == 1) {  /* 下键，查看更新的命令 */
        if (g_shell.history_browse_index > -1) {
            g_shell.history_browse_index--;
        }
    }

    /* 清除当前行 */
    shell_clear_line();

    /* 显示历史命令 */
    if (g_shell.history_browse_index >= 0) {
        /* 计算实际的历史记录索引 */
        int actual_idx = (g_shell.history_write_index - 1 - g_shell.history_browse_index
                         + SHELL_HISTORY_SIZE) % SHELL_HISTORY_SIZE;

        /* 复制到命令缓冲区 */
        strcpy(g_shell.cmd_buffer, g_shell.history[actual_idx]);
        g_shell.cmd_index = strlen(g_shell.cmd_buffer);

        /* 显示 */
        shell_printf("%s", g_shell.cmd_buffer);
    } else {
        /* 回到空行 */
        g_shell.cmd_buffer[0] = '\0';
        g_shell.cmd_index = 0;
    }
}

/**
 * @brief   获取历史记录数量
 * @return  当前历史记录数量
 */
int shell_history_get_count(void)
{
    return g_shell.history_count;
}

/**
 * @brief   获取指定索引的历史记录
 * @param   index: 索引（0=最新）
 * @return  历史命令字符串
 */
const char *shell_history_get(int index)
{
    if (index < 0 || index >= (int)g_shell.history_count) {
        return NULL;
    }

    int actual_idx = (g_shell.history_write_index - 1 - index + SHELL_HISTORY_SIZE)
                     % SHELL_HISTORY_SIZE;
    return g_shell.history[actual_idx];
}
#endif /* SHELL_HISTORY_ENABLE */

/*===========================================================================*/
/*                              TAB补全函数实现                                */
/*===========================================================================*/

#if SHELL_TAB_COMPLETE_ENABLE
/**
 * @brief   执行TAB自动补全
 *
 * @details 根据当前输入查找匹配的命令：
 *          - 无匹配：不做任何操作
 *          - 单个匹配：直接补全
 *          - 多个匹配：显示所有匹配项
 */
void shell_tab_complete(void)
{
    int match_count = 0;            /* 匹配数量 */
    const shell_cmd_t *match = NULL; /* 单个匹配时保存命令指针 */
    int total_cmds = shell_get_cmd_count();
    int input_len = g_shell.cmd_index;

    /* 空输入时显示所有命令 */
    if (input_len == 0) {
        shell_printf("\r\n");
        for (int i = 0; i < total_cmds; i++) {
            const shell_cmd_t *cmd = shell_get_cmd_by_index(i);
            if (cmd) {
                shell_printf("%s  ", cmd->name);
            }
        }
        shell_printf("\r\n");
        shell_show_prompt();
        return;
    }

    /* 查找所有匹配的命令 */
    for (int i = 0; i < total_cmds; i++) {
        const shell_cmd_t *cmd = shell_get_cmd_by_index(i);
        if (cmd && strncmp(g_shell.cmd_buffer, cmd->name, input_len) == 0) {
            match = cmd;
            match_count++;
        }
    }

    /* 根据匹配数量进行处理 */
    if (match_count == 0) {
        /* 无匹配，提示音或不做操作 */
        return;
    } else if (match_count == 1 && match != NULL) {
        /* 单个匹配，直接补全 */
        strcpy(g_shell.cmd_buffer, match->name);
        g_shell.cmd_index = strlen(g_shell.cmd_buffer);

        /* 添加空格便于输入参数 */
        if (g_shell.cmd_index < SHELL_CMD_BUFFER_SIZE - 1) {
            g_shell.cmd_buffer[g_shell.cmd_index++] = ' ';
            g_shell.cmd_buffer[g_shell.cmd_index] = '\0';
        }

        /* 清行并显示补全后的内容 */
        shell_clear_line();
        shell_printf("%s", g_shell.cmd_buffer);
    } else {
        /* 多个匹配，显示所有匹配项 */
        shell_printf("\r\n");
        for (int i = 0; i < total_cmds; i++) {
            const shell_cmd_t *cmd = shell_get_cmd_by_index(i);
            if (cmd && strncmp(g_shell.cmd_buffer, cmd->name, input_len) == 0) {
                shell_printf("%s  ", cmd->name);
            }
        }
        shell_printf("\r\n");
        shell_show_prompt();
        shell_printf("%s", g_shell.cmd_buffer);
    }
}
#endif /* SHELL_TAB_COMPLETE_ENABLE */

/*===========================================================================*/
/*                              内部辅助函数                                   */
/*===========================================================================*/

/**
 * @brief   显示Shell提示符
 */
static void shell_show_prompt(void)
{
    shell_printf(SHELL_PROMPT);
}

/**
 * @brief   清除当前行
 * @details 使用ANSI转义序列清除行并重新显示提示符
 */
static void shell_clear_line(void)
{
#if SHELL_ANSI_ENABLE
    shell_printf("\r\033[K" SHELL_PROMPT);
#else
    /* 不支持ANSI时，使用退格和空格覆盖 */
    shell_putc('\r');
    for (int i = 0; i < g_shell.cmd_index + sizeof(SHELL_PROMPT); i++) {
        shell_putc(' ');
    }
    shell_putc('\r');
    shell_show_prompt();
#endif
}

/**
 * @brief   解析命令行参数
 * @details 将命令行按空格分割成多个参数
 *          结果存储在g_shell.argc和g_shell.argv中
 */
static void shell_parse_args(void)
{
    g_shell.argc = 0;
    char *p = g_shell.cmd_buffer;
    char *token_start = NULL;

    /* 状态机解析：跳过前导空格，分割参数 */
    while (*p != '\0' && g_shell.argc < SHELL_MAX_ARGC) {
        /* 跳过空格 */
        while (*p == ' ' && *p != '\0') {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        /* 记录参数起始位置 */
        token_start = p;

        /* 查找参数结束位置 */
        while (*p != ' ' && *p != '\0') {
            p++;
        }

        /* 保存参数 */
        g_shell.argv[g_shell.argc++] = token_start;

        /* 在空格处截断 */
        if (*p == ' ') {
            *p = '\0';
            p++;
        }
    }
}

/**
 * @brief   执行当前命令
 * @details 解析参数并查找执行对应的命令函数
 */
static void shell_execute(void)
{
    /* 换行 */
    shell_printf("\r\n");

    /* 空命令检查 */
    if (g_shell.cmd_buffer[0] == '\0') {
        shell_show_prompt();
        return;
    }

#if SHELL_HISTORY_ENABLE
    /* 添加到历史记录 */
    shell_history_add(g_shell.cmd_buffer);
#endif

    /* 解析参数 */
    shell_parse_args();

    if (g_shell.argc == 0) {
        shell_show_prompt();
        return;
    }

    /* 查找命令 */
    const shell_cmd_t *cmd = shell_find_cmd(g_shell.argv[0]);

    /* 更新LCD显示当前命令 */
    shell_port_lcd_set_cmd(g_shell.cmd_buffer);

    if (cmd != NULL) {
        /* 执行命令 */
        g_shell.state = SHELL_STATE_EXECUTE;

        /* 更新LCD显示命令正在执行 */
        shell_port_lcd_set_output("Executing...");

        cmd->function();
        g_shell.state = SHELL_STATE_IDLE;

        /* 更新LCD显示执行完成 */
        shell_port_lcd_set_output("Done");
    } else {
        /* 未知命令 */
        shell_printf("Unknown command: %s\r\n", g_shell.argv[0]);
        shell_printf("Type 'help' for available commands.\r\n");

        /* 更新LCD显示错误 */
        shell_port_lcd_set_output("Unknown cmd!");
    }

    /* 显示提示符 */
    shell_printf("\r\n");
    shell_show_prompt();
}

/**
 * @brief   处理ESC序列
 * @param   c: 序列中的字符
 *
 * @details ESC序列格式: ESC [ 码
 *          例如: ESC [ A = 上键
 */
static void shell_process_esc_seq(char c)
{
    switch (g_shell.esc_seq_state) {
        case 1:  /* 已收到 ESC */
            if (c == '[') {
                g_shell.esc_seq_state = 2;  /* 等待方向键码 */
            } else {
                g_shell.esc_seq_state = 0;  /* 无效序列，重置 */
            }
            break;

        case 2:  /* 已收到 ESC [ */
            switch (c) {
                case 'A':  /* 上键 */
#if SHELL_HISTORY_ENABLE
                    shell_history_browse(-1);
#endif
                    break;

                case 'B':  /* 下键 */
#if SHELL_HISTORY_ENABLE
                    shell_history_browse(1);
#endif
                    break;

                case 'C':  /* 右键 - 暂不处理 */
                    break;

                case 'D':  /* 左键 - 暂不处理 */
                    break;

                default:
                    break;
            }
            g_shell.esc_seq_state = 0;  /* 重置状态 */
            break;

        default:
            g_shell.esc_seq_state = 0;
            break;
    }
}

/**
 * @brief   处理单个输入字符
 * @param   c: 输入字符
 *
 * @details 根据字符类型进行不同处理：
 *          - 可打印字符：添加到缓冲区
 *          - 回车/换行：执行命令
 *          - 退格/DEL：删除字符
 *          - TAB：自动补全
 *          - ESC：开始ESC序列
 */
static void shell_process_char(char c)
{
    /* ESC序列处理状态机 */
    if (g_shell.esc_seq_state > 0) {
        shell_process_esc_seq(c);
        return;
    }

    /* 根据字符类型处理 */
    switch (c) {
        case '\r':
        case '\n':
            /* 回车/换行：执行命令 */
            g_shell.cmd_buffer[g_shell.cmd_index] = '\0';
            shell_execute();
            g_shell.cmd_index = 0;
            g_shell.cmd_buffer[0] = '\0';
#if SHELL_HISTORY_ENABLE
            g_shell.history_browse_index = -1;  /* 重置历史浏览 */
#endif
            break;

        case '\b':
        case 127:  /* DEL */
            /* 退格键：删除一个字符 */
            if (g_shell.cmd_index > 0) {
                g_shell.cmd_index--;
                g_shell.cmd_buffer[g_shell.cmd_index] = '\0';
#if SHELL_ECHO_ENABLE
                shell_printf("\b \b");  /* 退格、空格覆盖、再退格 */
#endif
            }
            break;

        case '\t':
            /* TAB键：自动补全 */
#if SHELL_TAB_COMPLETE_ENABLE
            shell_tab_complete();
#endif
            break;

        case 27:  /* ESC */
            /* ESC键：开始ESC序列 */
            g_shell.esc_seq_state = 1;
            break;

        default:
            /* 可打印字符 */
            if (c >= 32 && c < 127) {
                if (g_shell.cmd_index < SHELL_CMD_BUFFER_SIZE - 1) {
                    g_shell.cmd_buffer[g_shell.cmd_index++] = c;
                    g_shell.cmd_buffer[g_shell.cmd_index] = '\0';
#if SHELL_ECHO_ENABLE
                    shell_putc(c);  /* 回显 */
#endif
                }
            }
            break;
    }
}

/*===========================================================================*/
/*                              核心API实现                                    */
/*===========================================================================*/

/**
 * @brief   Shell初始化
 * @details 初始化Shell状态，显示欢迎信息，准备接收命令
 */
void shell_init(void)
{
    /* 初始化Shell状态 */
    memset(&g_shell, 0, sizeof(shell_t));
    g_shell.state = SHELL_STATE_IDLE;
    g_shell.echo_enable = SHELL_ECHO_ENABLE;
#if SHELL_HISTORY_ENABLE
    g_shell.history_browse_index = -1;
#endif

    /* 显示欢迎信息 */
#if SHELL_BANNER_ENABLE
    shell_printf("\r\n");
    shell_printf("======================================\r\n");
    shell_printf("    %s %s\r\n", SHELL_NAME, SHELL_VERSION_STRING);
    shell_printf("    Build: %s %s\r\n", __DATE__, __TIME__);
    shell_printf("    Type 'help' for commands\r\n");
    shell_printf("======================================\r\n");
#endif

    /* 显示提示符 */
    shell_show_prompt();

    /* 标记为已初始化 */
    g_shell.initialized = 1;
}

/**
 * @brief   Shell主任务
 * @details 目前此函数为空，因为字符处理在shell_input_char中完成
 *          保留此函数用于将来可能的轮询模式支持
 */
void shell_task(void)
{
    /* 当前实现中，所有处理在shell_input_char中完成 */
    /* 此函数可用于轮询接收模式或其他周期性任务 */
}

/**
 * @brief   输入单个字符
 * @param   c: 输入字符
 */
void shell_input_char(char c)
{
    shell_process_char(c);
}

/**
 * @brief   输入字符串
 * @param   str: 字符串指针
 * @param   len: 字符串长度
 */
void shell_input_string(const char *str, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        shell_process_char(str[i]);
    }
}

/*===========================================================================*/
/*                              内置命令实现                                   */
/*===========================================================================*/

#if SHELL_CMD_HELP_ENABLE
/**
 * @brief   help命令实现
 * @details 显示所有可用命令及其描述
 */
static void shell_cmd_help(void)
{
    int total = shell_get_cmd_count();

    shell_printf("Available commands:\r\n");
    shell_printf("----------------------------------------\r\n");

    for (int i = 0; i < total; i++) {
        const shell_cmd_t *cmd = shell_get_cmd_by_index(i);
        if (cmd) {
            shell_printf("  %-12s - %s\r\n", cmd->name, cmd->description);
        }
    }

    shell_printf("----------------------------------------\r\n");
    shell_printf("Tips: Use TAB for auto-complete, UP/DOWN for history\r\n");
}
#endif

#if SHELL_CMD_VERSION_ENABLE
/**
 * @brief   version命令实现
 * @details 显示Shell版本信息
 */
static void shell_cmd_version(void)
{
    shell_printf("==============================\r\n");
    shell_printf("  %s %s\r\n", SHELL_NAME, SHELL_VERSION_STRING);
    shell_printf("  Build: %s %s\r\n", __DATE__, __TIME__);

#if defined(SHELL_PLATFORM_STM32G4)
    shell_printf("  Platform: STM32G4\r\n");
#elif defined(SHELL_PLATFORM_STM32F1)
    shell_printf("  Platform: STM32F1\r\n");
#elif defined(SHELL_PLATFORM_STM32F4)
    shell_printf("  Platform: STM32F4\r\n");
#else
    shell_printf("  Platform: Unknown\r\n");
#endif

    shell_printf("==============================\r\n");
}
#endif

#if SHELL_CMD_CLEAR_ENABLE
/**
 * @brief   clear命令实现
 * @details 清除终端屏幕
 */
static void shell_cmd_clear(void)
{
#if SHELL_ANSI_ENABLE
    shell_printf("\033[2J\033[H");  /* ANSI清屏并移到左上角 */
#else
    /* 不支持ANSI时，输出多个换行 */
    for (int i = 0; i < 50; i++) {
        shell_printf("\r\n");
    }
#endif
}
#endif

#if SHELL_CMD_ECHO_ENABLE
/**
 * @brief   echo命令实现
 * @details 回显输入的文本
 */
static void shell_cmd_echo(void)
{
    /* 跳过"echo "前缀，直接输出后续内容 */
    const char *cmdline = shell_get_cmdline();
    if (strlen(cmdline) > 5) {
        shell_printf("%s\r\n", cmdline + 5);
    } else {
        shell_printf("\r\n");
    }
}
#endif

#if SHELL_HISTORY_ENABLE && SHELL_CMD_HISTORY_ENABLE
/**
 * @brief   history命令实现
 * @details 显示命令历史记录
 */
static void shell_cmd_history(void)
{
    int count = shell_history_get_count();

    shell_printf("Command History:\r\n");

    if (count == 0) {
        shell_printf("  (empty)\r\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        const char *cmd = shell_history_get(i);
        if (cmd) {
            shell_printf("  %d: %s\r\n", i + 1, cmd);
        }
    }
}
#endif
