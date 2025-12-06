# Xifeng Shell 可移植架构设计文档

## 一、现有架构分析

### 1.1 当前架构问题

```
┌─────────────────────────────────────────────────────────────┐
│                    现有架构 (紧耦合)                          │
├─────────────────────────────────────────────────────────────┤
│  uart_app.c                                                 │
│  ├── Shell核心逻辑                                           │
│  ├── 命令解析                                                │
│  ├── TAB补全                                                │
│  ├── 历史记录                                                │
│  ├── 21个命令实现 ←──── 全部耦合在一起!                       │
│  │   ├── LED命令 (依赖ucLed数组)                             │
│  │   ├── ADC命令 (依赖adc_value)                             │
│  │   ├── RTC命令 (依赖HAL_RTC_*)                             │
│  │   ├── PWM命令 (依赖tim_app)                               │
│  │   └── ...                                                │
│  └── UART回调 (直接依赖STM32 HAL)                            │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 存在的问题

1. **硬件强耦合**: Shell核心直接调用STM32 HAL库函数
2. **命令与核心混合**: 所有命令实现都在uart_app.c中
3. **全局变量依赖**: 直接使用`ucLed[]`, `adc_value[]`等全局变量
4. **缺乏抽象层**: 没有可移植的硬件抽象接口
5. **注册机制缺失**: 无法动态添加/删除命令
6. **配置不灵活**: 缓冲区大小、命令数量等硬编码

---

## 二、优化后的可移植架构

### 2.1 架构总览

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        应用层 (Application Layer)                        │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐   │
│  │ LED Commands │ │ ADC Commands │ │ PWM Commands │ │ User Commands│   │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └──────┬───────┘   │
│         │                │                │                │           │
│         └────────────────┴────────────────┴────────────────┘           │
│                                   │                                     │
│                      ┌────────────▼────────────┐                        │
│                      │   命令注册表 (动态)      │                        │
│                      │   shell_cmd_register()  │                        │
│                      └────────────┬────────────┘                        │
├───────────────────────────────────┼─────────────────────────────────────┤
│                        Shell核心层 (Platform Independent)               │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                      shell_core.c / shell_core.h                 │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │   │
│  │  │ 命令解析器   │  │ TAB补全引擎 │  │ 历史记录管理器           │  │   │
│  │  │ shell_parse │  │ shell_tab   │  │ shell_history           │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘  │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │   │
│  │  │ ESC序列处理  │  │ 行编辑器    │  │ 提示符管理              │  │   │
│  │  │ shell_esc   │  │ shell_line  │  │ shell_prompt            │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────────────┤
│                        硬件抽象层 (HAL Abstraction Layer)               │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │                    shell_port.h (统一接口定义)                    │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │   │
│  │  │ shell_putc  │  │ shell_getc  │  │ shell_get_tick          │  │   │
│  │  │ shell_puts  │  │ shell_kbhit │  │ shell_delay_ms          │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────────────────┤
│                        平台移植层 (Platform Specific)                   │
│  ┌───────────────┐  ┌───────────────┐  ┌────────────────────────────┐  │
│  │ STM32G4 Port  │  │ STM32F1 Port  │  │ STM32F4 Port / Others...   │  │
│  │ shell_port_   │  │ shell_port_   │  │ shell_port_                │  │
│  │ stm32g4.c     │  │ stm32f1.c     │  │ stm32f4.c                  │  │
│  └───────────────┘  └───────────────┘  └────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 分层设计思维导图

```
                    ┌──────────────────────────────────────┐
                    │        Xifeng Shell v2.0             │
                    │         可移植架构设计                │
                    └──────────────────┬───────────────────┘
                                       │
        ┌──────────────────────────────┼──────────────────────────────┐
        │                              │                              │
        ▼                              ▼                              ▼
┌───────────────┐            ┌───────────────┐            ┌───────────────┐
│   配置层       │            │   核心层       │            │   移植层       │
│ shell_config.h│            │ shell_core    │            │ shell_port    │
└───────┬───────┘            └───────┬───────┘            └───────┬───────┘
        │                            │                            │
        ▼                            ▼                            ▼
┌───────────────┐    ┌───────────────────────────┐    ┌───────────────────┐
│• 缓冲区大小    │    │• 命令注册与查找            │    │• 字符输入输出      │
│• 历史记录数量  │    │• 命令解析执行              │    │• 时间获取          │
│• 提示符定义    │    │• TAB补全算法              │    │• 延时函数          │
│• 功能开关      │    │• 历史记录管理              │    │• 系统复位          │
│• 编译选项      │    │• ESC序列处理              │    │• 内存信息          │
└───────────────┘    │• 行编辑功能                │    └───────────────────┘
                     └───────────────────────────┘
                                       │
                                       ▼
                     ┌───────────────────────────┐
                     │       命令模块化           │
                     └───────────────────────────┘
                                       │
        ┌──────────────┬───────────────┼───────────────┬──────────────┐
        ▼              ▼               ▼               ▼              ▼
┌──────────────┐┌──────────────┐┌──────────────┐┌──────────────┐┌──────────────┐
│ shell_cmd_   ││ shell_cmd_   ││ shell_cmd_   ││ shell_cmd_   ││ shell_cmd_   │
│ builtin.c    ││ gpio.c       ││ system.c     ││ peripheral.c ││ user.c       │
├──────────────┤├──────────────┤├──────────────┤├──────────────┤├──────────────┤
│• help        ││• gpio        ││• reboot      ││• adc         ││• 用户自定义   │
│• version     ││• read        ││• uptime      ││• pwm         ││  命令        │
│• echo        ││• led         ││• mem         ││• freq        ││              │
│• clear       ││• toggle      ││• status      ││• eeprom      ││              │
│• history     ││              ││• delay       ││• time        ││              │
└──────────────┘└──────────────┘└──────────────┘└──────────────┘└──────────────┘
```

---

## 三、核心接口设计

### 3.1 移植层接口 (shell_port.h)

```c
/*===========================================================================
 * 移植层接口 - 用户需要实现这些函数来适配不同平台
 *===========================================================================*/

/* 字符输出函数 - 输出单个字符到终端 */
void shell_port_putc(char c);

/* 字符串输出函数 - 输出字符串到终端 */
void shell_port_puts(const char *str);

/* 格式化输出函数 - printf风格输出 */
int shell_port_printf(const char *fmt, ...);

/* 获取系统时钟节拍 - 返回毫秒数 */
uint32_t shell_port_get_tick(void);

/* 毫秒延时函数 */
void shell_port_delay_ms(uint32_t ms);

/* 系统复位函数 */
void shell_port_reboot(void);

/* 获取内存信息 (可选) */
void shell_port_get_mem_info(uint32_t *total, uint32_t *used);
```

### 3.2 命令注册接口

```c
/*===========================================================================
 * 命令注册宏 - 简化命令注册流程
 *===========================================================================*/

/* 静态命令注册宏 - 编译时注册 */
#define SHELL_CMD_EXPORT(name, func, desc)  \
    const shell_cmd_t __shell_cmd_##name    \
    __attribute__((used, section("shell_cmd"))) = { #name, desc, func }

/* 动态命令注册函数 - 运行时注册 */
int shell_cmd_register(const char *name, const char *desc, shell_cmd_func_t func);

/* 命令注销函数 */
int shell_cmd_unregister(const char *name);
```

### 3.3 Shell核心接口

```c
/*===========================================================================
 * Shell核心接口 - 主要API
 *===========================================================================*/

/* Shell初始化 */
void shell_init(void);

/* Shell主处理函数 - 在主循环中调用 */
void shell_task(void);

/* 输入单个字符到Shell */
void shell_input_char(char c);

/* 输入字符串到Shell */
void shell_input_string(const char *str, uint16_t len);

/* 获取Shell命令行参数 */
int shell_get_argc(void);
char *shell_get_argv(int index);
```

---

## 四、移植指南

### 4.1 移植步骤

```
步骤1: 拷贝核心文件
    ├── shell/
    │   ├── shell_core.c      (不需修改)
    │   ├── shell_core.h      (不需修改)
    │   ├── shell_config.h    (按需配置)
    │   └── shell_port.h      (接口定义)

步骤2: 创建平台移植文件
    └── port/
        └── shell_port_stm32xx.c  (实现shell_port.h中的函数)

步骤3: 配置shell_config.h
    ├── 设置缓冲区大小
    ├── 设置历史记录数量
    ├── 启用/禁用功能模块
    └── 配置提示符

步骤4: 初始化并调用
    main() {
        shell_init();
        while(1) {
            shell_task();  // 或在调度器中调用
        }
    }
```

### 4.2 STM32平台移植示例

```c
// shell_port_stm32.c

#include "shell_port.h"
#include "usart.h"  // STM32 HAL UART

/* 字符输出 */
void shell_port_putc(char c) {
    HAL_UART_Transmit(&huart1, (uint8_t*)&c, 1, 10);
}

/* 字符串输出 */
void shell_port_puts(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
}

/* 获取系统节拍 */
uint32_t shell_port_get_tick(void) {
    return HAL_GetTick();
}

/* 延时 */
void shell_port_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

/* 系统复位 */
void shell_port_reboot(void) {
    NVIC_SystemReset();
}
```

---

## 五、文件结构

```
STM32_Xifeng/
├── Shell/                          # Shell核心模块 (平台无关)
│   ├── Core/                       # 核心实现
│   │   ├── shell_core.c           # Shell主逻辑
│   │   ├── shell_core.h           # Shell核心头文件
│   │   ├── shell_parse.c          # 命令解析器
│   │   ├── shell_history.c        # 历史记录管理
│   │   ├── shell_tab.c            # TAB补全
│   │   └── shell_line.c           # 行编辑
│   │
│   ├── Config/                     # 配置文件
│   │   └── shell_config.h         # 用户配置
│   │
│   ├── Port/                       # 移植层
│   │   ├── shell_port.h           # 移植接口定义
│   │   ├── shell_port_stm32g4.c   # STM32G4移植
│   │   ├── shell_port_stm32f1.c   # STM32F1移植
│   │   └── shell_port_stm32f4.c   # STM32F4移植
│   │
│   └── Commands/                   # 命令模块
│       ├── shell_cmd_builtin.c    # 内置命令
│       ├── shell_cmd_gpio.c       # GPIO命令
│       ├── shell_cmd_system.c     # 系统命令
│       └── shell_cmd_peripheral.c # 外设命令
│
├── APP/                            # 应用层代码
├── Core/                           # STM32CubeMX生成代码
├── Drivers/                        # HAL驱动库
└── docs/                           # 文档
    ├── ARCHITECTURE.md            # 架构设计文档
    ├── PORTING_GUIDE.md           # 移植指南
    └── API_REFERENCE.md           # API参考
```

---

## 六、功能特性

### 6.1 核心特性
- [x] 命令行解析和执行
- [x] TAB键自动补全
- [x] 上下键历史记录浏览
- [x] 退格和删除键支持
- [x] ANSI转义序列支持
- [x] 命令动态注册/注销

### 6.2 可配置特性
- [x] 缓冲区大小可配置
- [x] 历史记录数量可配置
- [x] 提示符可自定义
- [x] 功能模块可裁剪
- [x] 命令回显可开关

### 6.3 扩展特性
- [ ] 命令别名支持
- [ ] 脚本执行支持
- [ ] 变量系统
- [ ] 命令重定向
- [ ] 多Shell实例

---

## 七、API使用示例

### 7.1 注册自定义命令

```c
// 方式1: 静态注册 (推荐)
void my_cmd_handler(void) {
    shell_printf("Hello from my command!\r\n");
}
SHELL_CMD_EXPORT(mycmd, my_cmd_handler, "My custom command");

// 方式2: 动态注册
void register_my_commands(void) {
    shell_cmd_register("test", "Test command", test_handler);
}
```

### 7.2 命令参数解析

```c
void led_cmd_handler(void) {
    int argc = shell_get_argc();

    if (argc < 2) {
        shell_printf("Usage: led <index> <on|off>\r\n");
        return;
    }

    char *led_idx = shell_get_argv(1);
    char *state = shell_get_argv(2);

    // 处理命令...
}
```

---

## 八、性能优化

### 8.1 内存优化
- 命令表使用const存储在Flash
- 可配置缓冲区大小适应不同RAM需求
- 支持静态/动态命令注册混合使用

### 8.2 执行效率
- 命令查找使用二分查找(排序后)
- 最小化中断处理时间
- 支持DMA接收

---

*文档版本: v2.0*
*更新日期: 2024*
