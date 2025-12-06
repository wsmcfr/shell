/**
 *******************************************************************************
 * @file    ringbuffer.c
 * @brief   环形缓冲区（循环队列）实现
 * @details 此文件实现了环形缓冲区的所有操作函数。
 *          环形缓冲区是一种FIFO（先进先出）数据结构，
 *          特别适合在中断和主循环之间传递数据。
 *
 * @version 1.0
 * @date    2024
 * @author  Xifeng Shell Team
 *
 * @par 设计说明:
 *      - 使用itemCount记录数据量，避免读写指针相等时的歧义
 *      - 读写指针采用取模运算实现循环
 *      - 不使用互斥锁，依赖单生产者-单消费者模型
 *******************************************************************************
 */

#include "ringbuffer.h"

/*===========================================================================*/
/*                              函数实现                                       */
/*===========================================================================*/

/**
 * @brief   初始化环形缓冲区
 * @param   rb: 环形缓冲区结构体指针
 *
 * @details 初始化操作包括：
 *          1. 将读指针和写指针设为0
 *          2. 将数据计数器清零
 *          3. 将缓冲区内容全部清零（可选，但有助于调试）
 *
 * @note    此函数应在使用缓冲区之前调用一次
 */
void ringbuffer_init(ringbuffer_t *rb)
{
    /* 复位读写指针到缓冲区起始位置 */
    rb->r = 0;
    rb->w = 0;

    /* 清空缓冲区内容（填充0） */
    memset(rb->buffer, 0, sizeof(uint8_t) * RINGBUFFER_SIZE);

    /* 重置数据计数器 */
    rb->itemCount = 0;
}

/**
 * @brief   检查环形缓冲区是否已满
 * @param   rb: 环形缓冲区结构体指针
 * @return  1=缓冲区已满, 0=缓冲区未满
 *
 * @details 通过比较itemCount和缓冲区大小判断是否已满
 *          当itemCount等于RINGBUFFER_SIZE时，缓冲区满
 *
 * @note    在写入数据前应调用此函数检查，防止数据丢失
 */
uint8_t ringbuffer_is_full(ringbuffer_t *rb)
{
    return (rb->itemCount == RINGBUFFER_SIZE);
}

/**
 * @brief   检查环形缓冲区是否为空
 * @param   rb: 环形缓冲区结构体指针
 * @return  1=缓冲区为空, 0=缓冲区非空
 *
 * @details 通过检查itemCount是否为0判断缓冲区是否为空
 *
 * @note    在读取数据前应调用此函数检查，避免读取无效数据
 */
uint8_t ringbuffer_is_empty(ringbuffer_t *rb)
{
    return (rb->itemCount == 0);
}

/**
 * @brief   向环形缓冲区写入数据
 * @param   rb:   环形缓冲区结构体指针
 * @param   data: 要写入的数据源指针
 * @param   num:  要写入的字节数
 * @return  0=写入成功, -1=写入失败（缓冲区已满）
 *
 * @details 写入过程：
 *          1. 检查缓冲区是否已满
 *          2. 逐字节写入数据到写指针位置
 *          3. 写指针循环递增（到达末尾回绕到开头）
 *          4. 更新数据计数器
 *
 *          写指针更新公式: w = (w + 1) % RINGBUFFER_SIZE
 *          这确保了当w到达数组末尾时，会自动回绕到0
 *
 * @warning 此函数在写入过程中不会再次检查是否溢出
 *          如果写入的数据量超过剩余空间，可能会覆盖未读取的数据
 *
 * @par 典型调用场景:
 *      在UART DMA接收完成回调中调用，将接收到的数据写入缓冲区
 */
int8_t ringbuffer_write(ringbuffer_t *rb, uint8_t *data, uint32_t num)
{
    /* 检查缓冲区是否已满 */
    if (ringbuffer_is_full(rb))
        return -1;

    /* 循环写入每个字节 */
    while (num--)
    {
        /* 将数据写入当前写指针位置 */
        rb->buffer[rb->w] = *data++;

        /* 写指针循环递增：到达末尾时回绕到开头 */
        rb->w = (rb->w + 1) % RINGBUFFER_SIZE;

        /* 增加数据计数 */
        rb->itemCount++;
    }

    return 0;  /* 写入成功 */
}

/**
 * @brief   从环形缓冲区读取数据
 * @param   rb:   环形缓冲区结构体指针
 * @param   data: 读取数据的目标缓冲区指针
 * @param   num:  要读取的字节数
 * @return  0=读取成功, -1=读取失败（缓冲区为空）
 *
 * @details 读取过程：
 *          1. 检查缓冲区是否为空
 *          2. 从读指针位置逐字节读取数据
 *          3. 读指针循环递增
 *          4. 更新数据计数器
 *
 *          读指针更新公式: r = (r + 1) % RINGBUFFER_SIZE
 *
 * @warning 此函数不检查请求读取的数量是否超过可用数据量
 *          调用前应确保itemCount >= num
 *
 * @par 典型调用场景:
 *      在主循环中周期性调用，处理缓冲区中累积的数据
 */
int8_t ringbuffer_read(ringbuffer_t *rb, uint8_t *data, uint32_t num)
{
    /* 检查缓冲区是否为空 */
    if (ringbuffer_is_empty(rb))
        return -1;

    /* 循环读取每个字节 */
    while (num--)
    {
        /* 从当前读指针位置读取数据 */
        *data++ = rb->buffer[rb->r];

        /* 读指针循环递增：到达末尾时回绕到开头 */
        rb->r = (rb->r + 1) % RINGBUFFER_SIZE;

        /* 减少数据计数 */
        rb->itemCount--;
    }

    return 0;  /* 读取成功 */
}
