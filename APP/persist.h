/**
 *******************************************************************************
 * @file    persist.h
 * @brief   掉电数据保存模块头文件
 * @details 提供数据持久化存储功能，使用外部EEPROM保存关键数据
 *
 * @version 1.0
 * @date    2024
 * @author  Shell Team
 *******************************************************************************
 */

#ifndef __PERSIST_H__
#define __PERSIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*===========================================================================*/
/*                              宏定义                                        */
/*===========================================================================*/

#define PERSIST_MAGIC           0xAA    /**< 数据有效标志 */
#define PERSIST_VERSION         0x01    /**< 数据版本号 */
#define PERSIST_EEPROM_ADDR     0x00    /**< EEPROM起始地址 */

/*===========================================================================*/
/*                              数据结构定义                                   */
/*===========================================================================*/

/**
 * @brief 持久化数据结构
 * @note  总大小40字节，存储在EEPROM地址0x00-0x27
 */
typedef struct {
    uint8_t  magic;           /**< 有效标志 0xAA */
    uint8_t  version;         /**< 版本号 0x01 */
    uint16_t crc;             /**< CRC16校验值 */
    uint8_t  led_state[8];    /**< LED状态 */
    int32_t  vara;            /**< 用户变量A */
    int32_t  varb;            /**< 用户变量B */
    int32_t  varc;            /**< 用户变量C */
    float    pwm_duty;        /**< PWM占空比 (0-100%) */
    int32_t  pwm_freq;        /**< PWM频率 (Hz) */
    /* RTC时间和日期 */
    uint8_t  rtc_hours;       /**< 小时 0-23 */
    uint8_t  rtc_minutes;     /**< 分钟 0-59 */
    uint8_t  rtc_seconds;     /**< 秒 0-59 */
    uint8_t  rtc_year;        /**< 年 0-99 (表示2000-2099) */
    uint8_t  rtc_month;       /**< 月 1-12 */
    uint8_t  rtc_day;         /**< 日 1-31 */
    uint8_t  rtc_weekday;     /**< 星期 1-7 */
    uint8_t  reserved;        /**< 保留字节，对齐用 */
} persist_data_t;

/*===========================================================================*/
/*                              函数声明                                       */
/*===========================================================================*/

/**
 * @brief  初始化掉电保存模块
 * @note   上电时调用，自动从EEPROM读取并恢复数据
 */
void persist_init(void);

/**
 * @brief  保存当前数据到EEPROM
 * @return 0=成功, -1=失败
 */
int persist_save(void);

/**
 * @brief  从EEPROM加载数据
 * @return 0=成功, -1=数据无效或校验失败
 */
int persist_load(void);

/**
 * @brief  清除保存的数据（恢复出厂设置）
 */
void persist_clear(void);

/**
 * @brief  检查EEPROM中是否有有效数据
 * @return 1=有效, 0=无效
 */
int persist_is_valid(void);

/**
 * @brief  获取当前保存的数据副本
 * @param  data: 输出数据指针
 */
void persist_get_data(persist_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __PERSIST_H__ */
