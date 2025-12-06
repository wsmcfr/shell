# Shell v2.0

一个轻量级、可移植的嵌入式Shell命令行系统，专为STM32系列MCU设计。

## 特性

- **可移植架构**: 分层设计，核心代码与硬件无关
- **丰富命令**: 21+内置命令，支持动态注册
- **交互友好**: TAB自动补全、上下键历史记录
- **高度可配置**: 功能模块可裁剪，适应不同RAM/Flash需求
- **详细注释**: 中文注释，便于学习和理解

## 架构概览

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (Application)                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │
│  │ System   │ │  GPIO    │ │Peripheral│ │  User    │   │
│  │ Commands │ │ Commands │ │ Commands │ │ Commands │   │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘   │
├─────────────────────────────────────────────────────────┤
│                    核心层 (Shell Core)                   │
│  命令解析 │ TAB补全 │ 历史记录 │ ESC序列处理             │
├─────────────────────────────────────────────────────────┤
│                    移植层 (Port Layer)                   │
│  ┌────────────┐ ┌────────────┐ ┌────────────────────┐  │
│  │ STM32G4    │ │ STM32F1    │ │ STM32F4 / Others   │  │
│  └────────────┘ └────────────┘ └────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## 目录结构

```
shell_project/
├── Shell/                      # Shell模块（可移植）
│   ├── Core/                   # 核心实现（平台无关）
│   │   ├── shell_core.c
│   │   └── shell_core.h
│   ├── Config/                 # 配置文件
│   │   └── shell_config.h
│   ├── Port/                   # 移植层
│   │   ├── shell_port.h        # 接口定义
│   │   └── shell_port_stm32g4.c
│   └── Commands/               # 命令模块
│       ├── shell_commands.h
│       ├── shell_cmd_system.c
│       ├── shell_cmd_gpio.c
│       └── shell_cmd_peripheral.c
├── APP/                        # 原有应用代码
├── Core/                       # STM32CubeMX生成
├── Drivers/                    # HAL驱动
├── docs/                       # 文档
│   ├── ARCHITECTURE.md         # 架构设计
│   └── PORTING_GUIDE.md        # 移植指南
└── MDK-ARM/                    # Keil工程
```

## 快速开始

### 1. 硬件需求
- STM32G431RBT6 开发板（或其他STM32）
- USB转串口模块
- 串口终端软件（推荐PuTTY/Xshell）

### 2. 波特率设置
- 默认: 9600bps
- 数据位: 8, 停止位: 1, 无校验

### 3. 串口连接
```
开发板         USB转串口
PA9 (TX)  -->  RX
PA10 (RX) -->  TX
GND       -->  GND
```

### 4. 运行效果
```
======================================
    Shell v2.0
    Build: Dec  6 2024 10:30:00
    Type 'help' for commands
======================================
> help
Available commands:
----------------------------------------
  help         - Show all available commands
  version      - Show shell version
  led          - Control LED: led <idx|all|status> [on|off]
  adc          - Read ADC values
  time         - Show/set RTC time
  pwm          - Set PWM: pwm <duty> [freq]
  ...
----------------------------------------
Tips: Use TAB for auto-complete, UP/DOWN for history
>
```

## 支持的命令

| 命令 | 功能 | 用法示例 |
|------|------|----------|
| help | 显示帮助 | `help` |
| version | 版本信息 | `version` |
| led | LED控制 | `led 0 on`, `led all off` |
| adc | ADC读取 | `adc` |
| time | RTC时间 | `time`, `time set 12:30:00` |
| pwm | PWM设置 | `pwm 50`, `pwm 75 1000` |
| gpio | GPIO控制 | `gpio A 5 1` |
| read | GPIO读取 | `read B 0` |
| toggle | LED翻转 | `toggle 0`, `toggle all` |
| key | 按键状态 | `key` |
| reboot | 系统复位 | `reboot` |
| uptime | 运行时间 | `uptime` |
| mem | 内存信息 | `mem` |
| status | 系统状态 | `status` |
| delay | 延时 | `delay 1000` |
| clear | 清屏 | `clear` |
| echo | 回显 | `echo Hello` |
| history | 历史记录 | `history` |
| freq | 频率测量 | `freq` |
| eeprom | EEPROM读写 | `eeprom r 0`, `eeprom w 0 255` |
| paraset | 变量设置 | `paraset vara 100` |

## 移植到其他平台

1. 复制 `Shell/` 目录到新工程
2. 修改 `shell_config.h` 中的平台配置
3. 创建 `shell_port_xxx.c` 实现移植接口
4. 详见 [移植指南](docs/PORTING_GUIDE.md)

## 添加自定义命令

```c
// 方式1: 动态注册
void my_cmd_handler(void) {
    shell_printf("Hello from my command!\r\n");
}

void app_init(void) {
    shell_cmd_register("mycmd", "My custom command", my_cmd_handler);
}

// 方式2: 获取参数
void led_cmd(void) {
    int argc = shell_get_argc();
    if (argc < 2) {
        shell_printf("Usage: myled <index>\r\n");
        return;
    }
    char *idx = shell_get_argv(1);
    // 处理参数...
}
```

## 配置选项

编辑 `shell_config.h` 可以：

- 调整缓冲区大小
- 启用/禁用特定命令
- 配置历史记录数量
- 选择目标平台
- 裁剪功能模块

## 文档

- [架构设计文档](docs/ARCHITECTURE.md) - 详细的架构说明和设计思路
- [移植指南](docs/PORTING_GUIDE.md) - 如何移植到其他STM32平台

## License

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！
