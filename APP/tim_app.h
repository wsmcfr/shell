#ifndef __TIM_APP_H__
#define __TIM_APP_H__

#include "bsp_system.h"

/* PWM设置函数 */
void pwm_set_duty(float Duty);
void pwm_set_frequency(int Frequency);

/* PWM获取函数 */
float pwm_get_duty(void);
int pwm_get_frequency(void);

/* 输入捕获处理 */
void ic_proc(void);

#endif
