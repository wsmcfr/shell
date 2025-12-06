/**
 *******************************************************************************
 * @file    lcd_app.h
 * @brief   LCD应用层头文件
 * @details 提供LCD显示界面的接口函数
 *
 * @version 2.0
 * @date    2024
 * @author  Shell Team
 *******************************************************************************
 */

#ifndef __LCD_APP_H__
#define __LCD_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp_system.h"

/*===========================================================================*/
/*                              宏定义                                        */
/*===========================================================================*/

/* LCD显示区域定义 */
#define LCD_AREA_TITLE      0       /* 标题区 */
#define LCD_AREA_TIME       1       /* 时间区 */
#define LCD_AREA_SHELL      2       /* Shell命令区 */
#define LCD_AREA_STATUS     3       /* 状态区 */
#define LCD_AREA_DATA       4       /* 数据区 */

/*===========================================================================*/
/*                              函数声明                                       */
/*===========================================================================*/

/**
 * @brief   LCD周期处理函数
 * @details 由调度器周期性调用，更新LCD显示内容
 */
void lcd_proc(void);

/**
 * @brief   设置LCD显示的Shell命令
 * @param   cmd: 命令字符串
 */
void lcd_set_shell_cmd(const char *cmd);

/**
 * @brief   设置LCD显示的Shell输出
 * @param   output: 输出字符串
 */
void lcd_set_shell_output(const char *output);

/**
 * @brief   LCD格式化显示函数
 * @param   Line:   行号
 * @param   format: 格式化字符串
 */
void LcdSprintf(uint8_t Line, char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_APP_H__ */
