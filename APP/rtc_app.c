#include "rtc_app.h"

RTC_TimeTypeDef time;  // 定义时间结构体
RTC_DateTypeDef date;  // 定义日期结构体

void rtc_proc(void)
{
    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
}
