#ifndef __HC_SR04_H
#define __HC_SR04_H

#include "stdint.h"
#include "main.h"
#include "tim.h"

#define TRIG_HIGH()  HAL_GPIO_WritePin(HC_SR04_TRIG_GPIO_Port, HC_SR04_TRIG_Pin, GPIO_PIN_SET)
#define TRIG_LOW()   HAL_GPIO_WritePin(HC_SR04_TRIG_GPIO_Port, HC_SR04_TRIG_Pin, GPIO_PIN_RESET)

void HC_SR04_Init(void);
float HC_SR04_MeasureDistance(void);

#endif