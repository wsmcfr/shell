#include "adc_app.h"

uint32_t dma_buff[2][30];//DMA接收缓存
float adc_value[2];//ADC采样值数组


void adc_proc(void)
{
    for(uint8_t i=0;i<30;i++)
    {
        adc_value[0] += (float)dma_buff[0][i];
        adc_value[1] += (float)dma_buff[1][i];
    }

    adc_value[0] = adc_value[0] / 30 * 3.3f / 4096;
    adc_value[1] = adc_value[1] / 30 * 3.3f / 4096;

}

