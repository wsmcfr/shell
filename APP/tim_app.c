#include "tim_app.h"

/**
 * @brief 设置PWM占空比
 *
 * 此函数用于设置TIM2的CH1通道的PWM占空比。占空比的范围为0%到100%。
 *
 * @param Duty 占空比，范围为0.0到100.0。
 */
void pwm_set_duty(float Duty)
{
    // 根据占空比计算捕获/比较寄存器的值
    TIM2->CCR2 = (TIM2->ARR + 1) * (Duty / 100.0f);
}

/**
 * @brief 设置PWM频率
 *
 * 此函数用于设置TIM2的PWM频率。频率的单位为Hz。
 *
 * @param Frequency 频率，单位为Hz。
 */
void pwm_set_frequency(int Frequency)
{
    // 获取定时器的时钟频率（假设TIM2使用的时钟频率为TIM2_CLK）
    uint32_t TIM2_CLK = 80000000;  // 假设72MHz, 需要根据实际情况调整

    // 保存旧的ARR值，用于计算当前占空比
    uint32_t old_ARR = TIM2->ARR;
    uint32_t old_CCR2 = TIM2->CCR2;

    // 根据所需频率计算自动重装载寄存器的值
    uint32_t ARR_Value = (TIM2_CLK / Frequency) - 1;

    // 计算当前占空比（使用旧的ARR值）
    float current_duty = 0.0f;
    if (old_ARR > 0) {
        current_duty = (float)old_CCR2 / (float)(old_ARR + 1);
    }

    // 设置自动重装载寄存器
    TIM2->ARR = ARR_Value;

    // 根据当前占空比计算新的CCR2值
    TIM2->CCR2 = (uint32_t)((ARR_Value + 1) * current_duty);

    // 产生更新事件，刷新寄存器
    TIM2->EGR = TIM_EGR_UG;
}



uint32_t tim_ic_buffer[64];// 用于存储输入捕获值的缓冲区
uint32_t tim_ic_val = 0;// 最终计算得到的输入捕获值
uint32_t tim_ic_temp = 0;// 临时存储输入捕获的中间值

/**
 * @brief 获取当前PWM占空比
 *
 * @return 占空比，范围为0.0到100.0
 */
float pwm_get_duty(void)
{
    if (TIM2->ARR == 0) {
        return 0.0f;
    }
    return (float)TIM2->CCR2 / (float)(TIM2->ARR + 1) * 100.0f;
}

/**
 * @brief 获取当前PWM频率
 *
 * @return 频率，单位为Hz
 */
int pwm_get_frequency(void)
{
    uint32_t TIM2_CLK = 80000000;  // 与pwm_set_frequency保持一致

    if (TIM2->ARR == 0) {
        return 0;
    }
    return (int)(TIM2_CLK / (TIM2->ARR + 1));
}

/**
 * @brief 处理输入捕获数据
 *
 * 此函数用于处理输入捕获数据。它计算tim_ic_buffer数组中捕获值的平均值，
 * 然后将其转换为相应的频率值（单位为Hz），并存储在tim_ic_val中。
 * 最后，调用limit_value函数将tim_ic_val限制在1到20000之间。
 */
void ic_proc(void)
{
    tim_ic_temp = 0;

    // 对缓冲区中的捕获值求和
    for (int i = 0; i < 64; i++)
    {
        tim_ic_temp += tim_ic_buffer[i];
    }

    // 计算平均值
    tim_ic_temp /= 64;

    // 计算频率值，单位为Hz
    tim_ic_val = (int)((float)(1000.0f * 1000.0f) / (float)tim_ic_temp);

    // 将计算得到的频率值限制在1到20000之间
    limit_value(&tim_ic_val, 1, 500, 20000);
}
