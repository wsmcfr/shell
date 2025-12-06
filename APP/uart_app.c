/**
 *******************************************************************************
 * @file    uart_app.c
 * @brief   UART应用层实现
 * @details 此文件负责UART数据接收和Shell模块的对接
 *
 * @version 2.0
 * @date    2024
 * @author  Xifeng Shell Team
 *
 * @note    使用新的Shell v2.0模块，原有Shell代码已移除
 *******************************************************************************
 */

#include "uart_app.h"
#include <string.h>

/* 包含新的Shell模块头文件 */
#include "shell_core.h"
#include "shell_commands.h"

/*===========================================================================*/
/*                              宏定义                                        */
/*===========================================================================*/

#define UART_READ_BUFFER_SIZE   64

/*===========================================================================*/
/*                              全局变量定义                                   */
/*===========================================================================*/

/**
 * @brief   UART接收环形缓冲区
 */
ringbuffer_t usart_rb;

/**
 * @brief   UART读取缓冲区（用于从环形缓冲区读取数据）
 */
static uint8_t usart_read_buffer[UART_READ_BUFFER_SIZE];

/**
 * @brief   可通过paraset命令设置的全局变量
 */
int vara = 0;
int varb = 0;
int varc = 0;

/*===========================================================================*/
/*                              UART回调函数                                   */
/*===========================================================================*/

/**
 * @brief   UART DMA接收完成回调函数
 * @param   huart: UART句柄
 * @param   Size:  接收到的数据大小
 *
 * @details 当UART DMA接收到数据时，此函数被调用
 *          将接收到的数据写入环形缓冲区，供uart_proc()处理
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (!ringbuffer_is_full(&usart_rb))
    {
        ringbuffer_write(&usart_rb, uart_rx_dma_buffer, Size);
    }
    memset(uart_rx_dma_buffer, 0, sizeof(uart_rx_dma_buffer));
}

/*===========================================================================*/
/*                              UART处理函数                                   */
/*===========================================================================*/

/**
 * @brief   UART数据处理函数
 *
 * @details 从环形缓冲区读取数据，逐字符传递给Shell模块处理
 *          此函数由调度器周期性调用（10ms周期）
 *
 * @note    新版本使用shell_input_char()将字符传递给Shell核心处理
 */
void uart_proc(void)
{
    /* 检查环形缓冲区是否有数据 */
    if (ringbuffer_is_empty(&usart_rb))
    {
        return;
    }

    /* 读取所有可用数据 */
    uint16_t available_data = usart_rb.itemCount;
    ringbuffer_read(&usart_rb, usart_read_buffer, available_data);

    /* 逐字符传递给Shell模块处理 */
    for (uint16_t i = 0; i < available_data; i++)
    {
        shell_input_char((char)usart_read_buffer[i]);
    }

    /* 清空读取缓冲区 */
    memset(usart_read_buffer, 0, sizeof(usart_read_buffer));
}
