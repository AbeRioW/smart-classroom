#include "dht11.h"
#include "delay.h"

#define DHT11_GPIO_IN()  do{GPIO_InitTypeDef GPIO_InitStruct = {0}; GPIO_InitStruct.Pin = DHT11_Pin; GPIO_InitStruct.Mode = GPIO_MODE_INPUT; GPIO_InitStruct.Pull = GPIO_PULLUP; HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);}while(0)
#define DHT11_GPIO_OUT() do{GPIO_InitTypeDef GPIO_InitStruct = {0}; GPIO_InitStruct.Pin = DHT11_Pin; GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; GPIO_InitStruct.Pull = GPIO_PULLUP; GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);}while(0)
#define DHT11_DQ_HIGH  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET)
#define DHT11_DQ_LOW   HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET)
#define DHT11_DQ_READ  HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)

static void DHT11_Rst(void)
{
    DHT11_GPIO_OUT();
    DHT11_DQ_LOW;
    delay_ms(20);
    DHT11_DQ_HIGH;
}

static uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    DHT11_GPIO_IN();
    while (DHT11_DQ_READ && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    if (retry >= 100)
        return 1;
    else
        retry = 0;
    while (!DHT11_DQ_READ && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    if (retry >= 100)
        return 1;
    return 0;
}

static uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;
    while (DHT11_DQ_READ && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    retry = 0;
    while (!DHT11_DQ_READ && retry < 100)
    {
        retry++;
        delay_us(1);
    }
    delay_us(40);
    if (DHT11_DQ_READ)
        return 1;
    else
        return 0;
}

static uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat;
    dat = 0;
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

void DHT11_Init(void)
{
    DHT11_Rst();
}

uint8_t DHT11_Read_Data(DHT11_Data_TypeDef *data)
{
    uint8_t buf[5];
    uint8_t i;
    DHT11_Rst();
    if (DHT11_Check() == 0)
    {
        for (i = 0; i < 5; i++)
        {
            buf[i] = DHT11_Read_Byte();
        }
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            data->humidity_int = buf[0];
            data->humidity_dec = buf[1];
            data->temperature_int = buf[2];
            data->temperature_dec = buf[3];
            data->check_sum = buf[4];
            return 0;
        }
    }
    return 1;
}