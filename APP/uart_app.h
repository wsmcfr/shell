/**
 *******************************************************************************
 * @file    uart_app.h
 * @brief   UART应用层头文件
 * @details 此文件声明UART应用层的接口函数
 *
 * @version 2.0
 * @date    2024
 * @author  Shell Team
 *******************************************************************************
 */

#ifndef __UART_APP_H__
#define __UART_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_system.h"

/*===========================================================================*/
/*                              函数声明                                       */
/*===========================================================================*/

/**
 * @brief   UART数据处理函数
 * @details 从环形缓冲区读取数据并传递给Shell模块
 *          由调度器周期性调用
 */
void uart_proc(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_APP_H__ */
