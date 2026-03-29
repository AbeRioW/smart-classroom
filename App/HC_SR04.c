#include "HC_SR04.h"
#include "delay.h"

volatile uint32_t g_echo_start = 0;
volatile uint32_t g_echo_end = 0;
volatile uint8_t g_echo_flag = 0;

void HC_SR04_Init(void)
{
    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);
}

float HC_SR04_MeasureDistance(void)
{
    uint32_t echo_time;
    float distance;
    uint32_t start_time = HAL_GetTick();
    
    g_echo_flag = 0;
    
    TRIG_HIGH();
    delay_us(10);
    TRIG_LOW();
    
    while(g_echo_flag != 2)
    {
        if(HAL_GetTick() - start_time > 100)
        {
            return -1.0; // 测量超时
        }
    }
    
    if(g_echo_end >= g_echo_start)
    {
        echo_time = g_echo_end - g_echo_start;
    }
    else
    {
        echo_time = 0xFFFF - g_echo_start + g_echo_end + 1; // 处理计时器溢出
    }
    
    distance = echo_time * 34000.0 / (2 * 1000000.0);
    
    if(distance > 400.0 || distance < 2.0)
    {
        return -1.0; // 超出测量范围
    }
    
    return distance;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        if(g_echo_flag == 0)
        {
            g_echo_start = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
            g_echo_flag = 1;
        }
        else if(g_echo_flag == 1)
        {
            g_echo_end = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
            __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
            g_echo_flag = 2;
        }
    }
}