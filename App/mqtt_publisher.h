#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include "stm32f1xx_hal.h" // 根据实际MCU型号修改

// 外部变量声明（如果需要在其他地方访问）
// 外部变量声明（如果需要在其他地方访问）
extern uint32_t msg_id;


// 发布函数（供定时器中断调用）
void MQTT_Publish_Data(const char* key, const char* value);

// 从中断触发的接口：仅置标志（在 ISR 中调用）
void MQTT_Publish_TriggerFromISR(void);
void MQTT_Publish_Dataw(float temperature,  float value);

#endif