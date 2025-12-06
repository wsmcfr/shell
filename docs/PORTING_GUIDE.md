# Xifeng Shell 移植指南

## 一、移植概述

Xifeng Shell 采用分层架构设计，可以方便地移植到不同的STM32系列及其他MCU平台。移植工作主要集中在移植层（Port Layer），核心层代码无需修改。

### 1.1 需要移植的文件

```
需要修改:
├── Shell/Config/shell_config.h    # 配置RAM大小、平台选择等
└── Shell/Port/shell_port_xxx.c    # 创建新的移植文件

无需修改:
├── Shell/Core/shell_core.c        # Shell核心实现
├── Shell/Core/shell_core.h        # Shell核心头文件
├── Shell/Port/shell_port.h        # 移植层接口定义
└── Shell/Commands/*.c             # 命令实现
```

### 1.2 移植工作量估算

| 任务 | 代码量 | 难度 |
|------|--------|------|
| 基础输出函数 | ~20行 | 简单 |
| 系统函数 | ~30行 | 简单 |
| GPIO函数 | ~50行 | 中等 |
| 外设函数 | ~100行 | 中等 |
| **总计** | **~200行** | - |

---

## 二、移植步骤

### Step 1: 复制Shell模块

将以下目录复制到新工程：

```
Shell/
├── Core/
│   ├── shell_core.c
│   └── shell_core.h
├── Config/
│   └── shell_config.h
├── Port/
│   ├── shell_port.h
│   └── shell_port_xxx.c  (需要创建)
└── Commands/
    ├── shell_commands.h
    ├── shell_cmd_system.c
    ├── shell_cmd_gpio.c
    └── shell_cmd_peripheral.c
```

### Step 2: 修改配置文件

编辑 `shell_config.h`：

```c
/* 修改平台定义 */
// #define SHELL_PLATFORM_STM32G4
#define SHELL_PLATFORM_STM32F1    // 取消注释目标平台

/* 修改RAM大小 */
#define SHELL_PLATFORM_RAM_SIZE   (20 * 1024)  // 20KB for STM32F103C8

/* 修改RAM地址 */
#define SHELL_PLATFORM_RAM_START  0x20000000
#define SHELL_PLATFORM_RAM_END    (SHELL_PLATFORM_RAM_START + SHELL_PLATFORM_RAM_SIZE)
```

### Step 3: 创建移植文件

创建 `shell_port_stm32f1.c`：

```c
#include "shell_port.h"
#include "usart.h"  // HAL库头文件

/* ========== 基础输出函数 ========== */

void shell_port_putc(char c)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)&c, 1, 10);
}

void shell_port_puts(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
}

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

/* ========== 系统函数 ========== */

uint32_t shell_port_get_tick(void)
{
    return HAL_GetTick();
}

void shell_port_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void shell_port_reboot(void)
{
    NVIC_SystemReset();
}

void shell_port_get_mem_info(uint32_t *total, uint32_t *used)
{
    *total = SHELL_PLATFORM_RAM_SIZE;
    register uint32_t sp __asm("sp");
    *used = SHELL_PLATFORM_RAM_END - sp;
}

/* ========== GPIO函数 ========== */

// 根据具体硬件实现...
```

### Step 4: 添加到工程

1. 将Shell目录添加到工程包含路径
2. 将所有.c文件添加到编译
3. 配置UART外设（如需DMA接收）

### Step 5: 初始化调用

在 `main.c` 中：

```c
#include "shell_core.h"
#include "shell_commands.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    // 其他外设初始化...

    shell_init();           // 初始化Shell核心
    shell_commands_init();  // 注册所有命令

    while(1)
    {
        shell_task();
    }
}
```

---

## 三、各平台移植要点

### 3.1 STM32F1系列

**特点**:
- RAM: 20KB (STM32F103C8) / 64KB (STM32F103VE)
- 没有DMA Request Mux
- GPIO寄存器与G4略有不同

**注意事项**:
1. DMA配置不同，参考F1的HAL库文档
2. UART可能使用不同的DMA通道

```c
/* STM32F1 UART DMA配置示例 */
hdma_usart1_rx.Instance = DMA1_Channel5;  // F1使用Channel而非Request
```

### 3.2 STM32F4系列

**特点**:
- RAM: 192KB (STM32F407VG)
- 更高的CPU频率
- DMA Stream模式

**注意事项**:
```c
/* STM32F4 DMA配置 */
hdma_usart1_rx.Instance = DMA2_Stream2;  // F4使用Stream
hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
```

### 3.3 STM32H7系列

**特点**:
- 双核架构（部分型号）
- 多个RAM区域
- 需要注意Cache一致性

**注意事项**:
```c
/* STM32H7 需要处理D-Cache */
SCB_CleanDCache();  // DMA传输前
SCB_InvalidateDCache();  // DMA接收后
```

---

## 四、最小移植示例

如果只需要基本Shell功能，可以只实现以下函数：

```c
// 最小移植 - 只需5个函数

// 必须实现
void shell_port_putc(char c);
void shell_port_puts(const char *str);
int shell_port_printf(const char *fmt, ...);
uint32_t shell_port_get_tick(void);
void shell_port_delay_ms(uint32_t ms);

// 可选实现（不实现则对应命令不可用）
void shell_port_reboot(void);          // reboot命令
void shell_port_get_mem_info(...);     // mem命令
// GPIO/LED/ADC/PWM等函数...
```

**最小移植代码示例**:

```c
#include "shell_port.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;

void shell_port_putc(char c)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)&c, 1, HAL_MAX_DELAY);
}

void shell_port_puts(const char *str)
{
    while(*str) {
        shell_port_putc(*str++);
    }
}

int shell_port_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    shell_port_puts(buf);
    return len;
}

uint32_t shell_port_get_tick(void)
{
    return HAL_GetTick();
}

void shell_port_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}
```

---

## 五、数据接收方案

### 方案1: 轮询接收（最简单）

```c
// 在shell_task中轮询检查
void my_shell_task(void)
{
    uint8_t ch;
    if (HAL_UART_Receive(&huart1, &ch, 1, 0) == HAL_OK) {
        shell_input_char(ch);
    }
}
```

### 方案2: 中断接收（推荐）

```c
uint8_t rx_byte;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    shell_input_char(rx_byte);
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

// 初始化时启动接收
HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
```

### 方案3: DMA + 空闲中断（高效）

```c
uint8_t dma_buffer[64];

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    shell_input_string((char*)dma_buffer, Size);
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_buffer, sizeof(dma_buffer));
}

// 初始化时启动DMA接收
HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_buffer, sizeof(dma_buffer));
```

---

## 六、裁剪功能

### 6.1 禁用不需要的命令

编辑 `shell_config.h`：

```c
/* 禁用GPIO命令节省代码空间 */
#define SHELL_CMD_GPIO_ENABLE    0
#define SHELL_CMD_READ_ENABLE    0

/* 禁用EEPROM命令 */
#define SHELL_CMD_EEPROM_ENABLE  0
```

### 6.2 减少缓冲区大小

```c
/* 小RAM系统配置 */
#define SHELL_CMD_BUFFER_SIZE    64   // 减少到64字节
#define SHELL_HISTORY_SIZE       5    // 减少历史记录
#define SHELL_RX_BUFFER_SIZE     32   // 减少接收缓冲
```

### 6.3 禁用高级功能

```c
/* 禁用历史记录 */
#define SHELL_HISTORY_ENABLE     0

/* 禁用TAB补全 */
#define SHELL_TAB_COMPLETE_ENABLE 0

/* 禁用ANSI颜色 */
#define SHELL_ANSI_ENABLE        0
```

---

## 七、常见问题

### Q1: Shell无输出
- 检查UART波特率配置
- 检查shell_port_putc/puts实现
- 确认串口连接正确

### Q2: 输入无响应
- 检查数据接收方式是否正确
- 确认shell_input_char被调用
- 检查终端软件的换行符设置

### Q3: 上下键显示乱码
- 确认终端软件支持ANSI转义序列
- 推荐使用: PuTTY, SecureCRT, Xshell

### Q4: 编译错误
- 确认所有头文件路径已添加
- 检查HAL库版本兼容性
- 确认shell_config.h中的平台定义正确

---

## 八、移植检查清单

- [ ] shell_config.h 配置正确
- [ ] shell_port_xxx.c 创建完成
- [ ] shell_port_putc 测试通过
- [ ] shell_port_puts 测试通过
- [ ] shell_port_printf 测试通过
- [ ] shell_port_get_tick 返回正确
- [ ] shell_port_delay_ms 工作正常
- [ ] 数据接收方式配置正确
- [ ] help命令显示正常
- [ ] TAB补全工作正常
- [ ] 上下键历史工作正常

---

*文档版本: 2.0*
*更新日期: 2024*
