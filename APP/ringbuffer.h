/**
 *******************************************************************************
 * @file    ringbuffer.h
 * @brief   环形缓冲区（循环队列）头文件
 * @details 此模块实现了一个通用的环形缓冲区数据结构，主要用于：
 *          - UART DMA接收数据缓存
 *          - 生产者-消费者模式的数据传递
 *          - 解耦中断和主循环的数据处理
 *
 * @version 1.0
 * @date    2024
 * @author  Shell Team
 *
 * @par 环形缓冲区工作原理:
 *      环形缓冲区使用一个固定大小的数组，通过读指针(r)和写指针(w)
 *      实现循环利用空间。当指针到达数组末尾时，自动回绕到开头。
 *
 *      +---+---+---+---+---+---+---+---+
 *      | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |  <- 缓冲区
 *      +---+---+---+---+---+---+---+---+
 *            ^           ^
 *            r           w
 *        读指针       写指针
 *
 * @par 使用示例:
 * @code
 * ringbuffer_t rb;
 * uint8_t data[10];
 *
 * ringbuffer_init(&rb);                    // 初始化
 * ringbuffer_write(&rb, tx_data, 5);       // 写入5字节
 * ringbuffer_read(&rb, data, 3);           // 读取3字节
 * @endcode
 *******************************************************************************
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_system.h"

/*===========================================================================*/
/*                              宏定义                                        */
/*===========================================================================*/

/**
 * @brief   环形缓冲区大小（字节）
 * @note    根据实际需求调整，应大于单次最大传输量
 *          太小会导致数据丢失，太大会浪费RAM
 */
#define RINGBUFFER_SIZE (30)

/*===========================================================================*/
/*                              类型定义                                       */
/*===========================================================================*/

/**
 * @brief   环形缓冲区结构体
 * @details 包含缓冲区数据和管理信息
 */
typedef struct {
    uint32_t w;                             /**< 写指针位置（下一个写入位置） */
    uint32_t r;                             /**< 读指针位置（下一个读取位置） */
    uint8_t buffer[RINGBUFFER_SIZE];        /**< 数据存储数组 */
    uint32_t itemCount;                     /**< 当前缓冲区中的数据量 */
} ringbuffer_t;

/*===========================================================================*/
/*                              函数声明                                       */
/*===========================================================================*/

/**
 * @brief   初始化环形缓冲区
 * @param   rb: 环形缓冲区结构体指针
 * @details 将读写指针清零，清空缓冲区内容
 *
 * @note    使用缓冲区前必须调用此函数进行初始化
 */
void ringbuffer_init(ringbuffer_t *rb);

/**
 * @brief   检查环形缓冲区是否已满
 * @param   rb: 环形缓冲区结构体指针
 * @return  1=已满, 0=未满
 *
 * @note    写入前应检查缓冲区是否已满，避免数据覆盖
 */
uint8_t ringbuffer_is_full(ringbuffer_t *rb);

/**
 * @brief   检查环形缓冲区是否为空
 * @param   rb: 环形缓冲区结构体指针
 * @return  1=为空, 0=非空
 *
 * @note    读取前应检查缓冲区是否有数据
 */
uint8_t ringbuffer_is_empty(ringbuffer_t *rb);

/**
 * @brief   向环形缓冲区写入数据
 * @param   rb:   环形缓冲区结构体指针
 * @param   data: 要写入的数据指针
 * @param   num:  要写入的字节数
 * @return  0=成功, -1=失败（缓冲区已满）
 *
 * @warning 此函数不检查是否会溢出，调用前应确保有足够空间
 *          或使用ringbuffer_is_full()检查
 */
int8_t ringbuffer_write(ringbuffer_t *rb, uint8_t *data, uint32_t num);

/**
 * @brief   从环形缓冲区读取数据
 * @param   rb:   环形缓冲区结构体指针
 * @param   data: 读取数据存放的缓冲区指针
 * @param   num:  要读取的字节数
 * @return  0=成功, -1=失败（缓冲区为空）
 *
 * @warning 调用前应确保缓冲区中有足够的数据
 */
int8_t ringbuffer_read(ringbuffer_t *rb, uint8_t *data, uint32_t num);

/*===========================================================================*/
/*                              全局变量声明                                   */
/*===========================================================================*/

/**
 * @brief   UART接收用的环形缓冲区实例
 * @details 用于存储UART DMA接收到的数据
 *          在uart_app.c中定义
 */
extern ringbuffer_t usart_rb;

#ifdef __cplusplus
}
#endif

#endif /* RINGBUFFER_H */
