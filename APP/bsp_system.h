#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H

#include "stdio.h"
#include "stdarg.h"
#include "string.h"

#include "rtc.h"
#include "lcd.h"
#include "main.h"
#include "usart.h"
#include "system.h"
#include "filter.h"
#include "scheduler.h"
#include "led_app.h"
#include "key_app.h"
#include "lcd_app.h"
#include "uart_app.h"
#include "adc_app.h"

/* 新Shell模块头文件 */
#include "shell_core.h"
#include "shell_commands.h"
#include "rtc_app.h"
#include "tim_app.h"
#include "ringbuffer.h"
#include "i2c_hal.h"
#include "persist.h"

extern uint16_t uart_rx_index;          // 接收缓冲区索引
extern uint32_t uart_rx_ticks;          // 接收缓冲时间戳
extern uint8_t uart_rx_buffer[128];     // 接收缓冲
extern uint8_t ucLed[8];                // LED 状态数组
extern uint8_t uart_rx_dma_buffer[128]; // 接收缓冲
extern uint32_t dma_buff[2][30];        // DMA接收缓冲
extern float adc_value[2];              // ADC采样值数组
extern RTC_TimeTypeDef time;            // 定义时间结构体
extern RTC_DateTypeDef date;            // 定义日期结构体
extern uint32_t tim_ic_buffer[64];      // 用于存储输入捕获值的缓冲区
extern uint32_t tim_ic_val;             // 最终计算得到的输入捕获值

extern int vara;
extern int varb;
extern int varc;


#endif


