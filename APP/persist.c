/**
 *******************************************************************************
 * @file    persist.c
 * @brief   掉电数据保存模块实现
 * @details 使用外部EEPROM (AT24C02) 保存关键数据，支持CRC校验
 *
 * @version 1.0
 * @date    2024
 * @author  Shell Team
 *******************************************************************************
 */

#include "persist.h"
#include "i2c_hal.h"
#include "led_app.h"
#include "tim_app.h"
#include "rtc.h"
#include <string.h>

/*===========================================================================*/
/*                              外部变量引用                                   */
/*===========================================================================*/

extern uint8_t ucLed[8];        /* LED状态数组 */
extern int vara, varb, varc;    /* 用户变量 */
extern RTC_HandleTypeDef hrtc;  /* RTC句柄 */

/*===========================================================================*/
/*                              私有变量                                       */
/*===========================================================================*/

static persist_data_t persist_cache;    /* 数据缓存 */

/*===========================================================================*/
/*                              私有函数                                       */
/*===========================================================================*/

/**
 * @brief  CRC16-MODBUS校验算法
 * @param  data: 数据指针
 * @param  len: 数据长度
 * @return CRC16校验值
 */
static uint16_t crc16_modbus(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief  计算数据结构的CRC值（不包含magic, version, crc字段）
 * @param  data: 数据结构指针
 * @return CRC16校验值
 */
static uint16_t persist_calc_crc(persist_data_t *data)
{
    /* 从led_state开始计算CRC，跳过magic(1) + version(1) + crc(2) = 4字节 */
    uint8_t *ptr = (uint8_t *)data + 4;
    uint16_t len = sizeof(persist_data_t) - 4;

    return crc16_modbus(ptr, len);
}

/**
 * @brief  AT24C02页大小
 * @note   AT24C02每页8字节，写入时不能跨页
 */
#define EEPROM_PAGE_SIZE    8

/**
 * @brief  从EEPROM读取数据
 * @param  data: 输出数据指针
 */
static void persist_read_eeprom(persist_data_t *data)
{
    eeprom_read((uint8_t *)data, PERSIST_EEPROM_ADDR, sizeof(persist_data_t));
}

/**
 * @brief  写入数据到EEPROM（分页写入）
 * @param  data: 输入数据指针
 * @note   AT24C02每页8字节，必须分页写入，否则数据会回绕覆盖
 */
static void persist_write_eeprom(persist_data_t *data)
{
    uint8_t *ptr = (uint8_t *)data;
    uint8_t addr = PERSIST_EEPROM_ADDR;
    uint16_t remaining = sizeof(persist_data_t);
    uint8_t write_len;

    while (remaining > 0) {
        /* 计算当前页剩余空间 */
        uint8_t page_offset = addr % EEPROM_PAGE_SIZE;
        uint8_t page_remaining = EEPROM_PAGE_SIZE - page_offset;

        /* 本次写入长度 = min(剩余数据, 页剩余空间) */
        write_len = (remaining < page_remaining) ? remaining : page_remaining;

        /* 写入当前页 */
        eeprom_write(ptr, addr, write_len);

        /* 更新指针和计数 */
        ptr += write_len;
        addr += write_len;
        remaining -= write_len;
    }
}

/**
 * @brief  收集当前系统数据
 * @param  data: 输出数据指针
 */
static void persist_collect_data(persist_data_t *data)
{
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

    /* 设置标志和版本 */
    data->magic = PERSIST_MAGIC;
    data->version = PERSIST_VERSION;

    /* 收集LED状态 */
    memcpy(data->led_state, ucLed, 8);

    /* 收集用户变量 */
    data->vara = vara;
    data->varb = varb;
    data->varc = varc;

    /* 收集PWM状态 */
    data->pwm_duty = pwm_get_duty();
    data->pwm_freq = pwm_get_frequency();

    /* 收集RTC时间和日期 */
    HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);

    data->rtc_hours = rtc_time.Hours;
    data->rtc_minutes = rtc_time.Minutes;
    data->rtc_seconds = rtc_time.Seconds;
    data->rtc_year = rtc_date.Year;
    data->rtc_month = rtc_date.Month;
    data->rtc_day = rtc_date.Date;
    data->rtc_weekday = rtc_date.WeekDay;
    data->reserved = 0;

    /* 计算CRC */
    data->crc = persist_calc_crc(data);
}

/**
 * @brief  恢复数据到系统变量
 * @param  data: 输入数据指针
 */
static void persist_restore_data(persist_data_t *data)
{
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

    /* 恢复LED状态 */
    memcpy(ucLed, data->led_state, 8);

    /* 恢复用户变量 */
    vara = data->vara;
    varb = data->varb;
    varc = data->varc;

    /* 恢复PWM设置 */
    pwm_set_frequency(data->pwm_freq);
    pwm_set_duty(data->pwm_duty);

    /* 恢复RTC时间和日期 */
    rtc_time.Hours = data->rtc_hours;
    rtc_time.Minutes = data->rtc_minutes;
    rtc_time.Seconds = data->rtc_seconds;
    rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    rtc_time.StoreOperation = RTC_STOREOPERATION_RESET;

    rtc_date.Year = data->rtc_year;
    rtc_date.Month = data->rtc_month;
    rtc_date.Date = data->rtc_day;
    rtc_date.WeekDay = data->rtc_weekday;

    HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
}

/*===========================================================================*/
/*                              公共函数                                       */
/*===========================================================================*/

/**
 * @brief  初始化掉电保存模块
 */
void persist_init(void)
{
    /* I2C已在GPIO初始化时配置，这里不需要额外初始化 */

    /* 尝试加载保存的数据 */
    if (persist_load() == 0) {
        /* 加载成功，数据已恢复 */
    } else {
        /* 加载失败，使用默认值 */
        memset(&persist_cache, 0, sizeof(persist_cache));
    }
}

/**
 * @brief  保存当前数据到EEPROM
 * @return 0=成功, -1=失败
 */
int persist_save(void)
{
    persist_data_t verify_data;

    /* 收集当前数据 */
    persist_collect_data(&persist_cache);

    /* 写入EEPROM */
    persist_write_eeprom(&persist_cache);

    /* 读回验证 */
    persist_read_eeprom(&verify_data);

    /* 比较验证 */
    if (memcmp(&persist_cache, &verify_data, sizeof(persist_data_t)) != 0) {
        return -1;  /* 写入验证失败 */
    }

    return 0;
}

/**
 * @brief  从EEPROM加载数据
 * @return 0=成功, -1=数据无效或校验失败
 */
int persist_load(void)
{
    persist_data_t data;

    /* 从EEPROM读取 */
    persist_read_eeprom(&data);

    /* 检查magic标志 */
    if (data.magic != PERSIST_MAGIC) {
        return -1;
    }

    /* 检查版本 */
    if (data.version != PERSIST_VERSION) {
        return -1;
    }

    /* 验证CRC */
    uint16_t calc_crc = persist_calc_crc(&data);
    if (data.crc != calc_crc) {
        return -1;
    }

    /* 数据有效，恢复到系统 */
    persist_restore_data(&data);

    /* 更新缓存 */
    memcpy(&persist_cache, &data, sizeof(persist_data_t));

    return 0;
}

/**
 * @brief  清除保存的数据（恢复出厂设置）
 */
void persist_clear(void)
{
    persist_data_t empty;
    RTC_TimeTypeDef rtc_time;
    RTC_DateTypeDef rtc_date;

    /* 清空数据 */
    memset(&empty, 0, sizeof(persist_data_t));

    /* 写入EEPROM */
    persist_write_eeprom(&empty);

    /* 清空缓存 */
    memset(&persist_cache, 0, sizeof(persist_cache));

    /* 重置系统变量为默认值 */
    memset(ucLed, 0, 8);
    vara = 0;
    varb = 0;
    varc = 0;
    pwm_set_frequency(1000);
    pwm_set_duty(50.0f);

    /* 重置RTC到默认时间 2024-01-01 00:00:00 */
    rtc_time.Hours = 0;
    rtc_time.Minutes = 0;
    rtc_time.Seconds = 0;
    rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    rtc_time.StoreOperation = RTC_STOREOPERATION_RESET;

    rtc_date.Year = 24;     /* 2024 */
    rtc_date.Month = 1;
    rtc_date.Date = 1;
    rtc_date.WeekDay = RTC_WEEKDAY_MONDAY;

    HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
}

/**
 * @brief  检查EEPROM中是否有有效数据
 * @return 1=有效, 0=无效
 */
int persist_is_valid(void)
{
    persist_data_t data;

    /* 从EEPROM读取 */
    persist_read_eeprom(&data);

    /* 检查magic和版本 */
    if (data.magic != PERSIST_MAGIC || data.version != PERSIST_VERSION) {
        return 0;
    }

    /* 验证CRC */
    uint16_t calc_crc = persist_calc_crc(&data);
    if (data.crc != calc_crc) {
        return 0;
    }

    return 1;
}

/**
 * @brief  获取当前保存的数据副本
 * @param  data: 输出数据指针
 */
void persist_get_data(persist_data_t *data)
{
    if (data != NULL) {
        memcpy(data, &persist_cache, sizeof(persist_data_t));
    }
}
