#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

typedef struct {
    uint8_t humidity_int;
    uint8_t humidity_dec;
    uint8_t temperature_int;
    uint8_t temperature_dec;
    uint8_t check_sum;
} DHT11_Data_TypeDef;

void DHT11_Init(void);
uint8_t DHT11_Read_Data(DHT11_Data_TypeDef *data);

#endif