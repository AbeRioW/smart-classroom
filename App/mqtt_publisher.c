#include "mqtt_publisher.h"
#include "delay.h"
#include "esp8266.h" // 包含ESP8266 MQTT相关函数
#include "usart.h"       // 包含UART相关函数
#include <stdio.h>
//#include <sys/_intsup.h>


// 模块内部变量
static uint32_t last_pub = 0;
#define PUB_INTERVAL_MS 3000 // 发布间隔，可根据需要修改

// 外部变量定义
uint32_t msg_id = 0;

// MQTT数据发布实现（应在主循环/线程上下文调用，绝不可在 ISR 中调用）
void MQTT_Publish_Data(const char* key, const char* value)
{
//    uint32_t now = HAL_GetTick();
//    
//    // 非阻塞定时检查
//    if ((uint32_t)(now - last_pub) < PUB_INTERVAL_MS) return;
//    last_pub = now;

    // 构造JSON payload
    char payload[256];
    int n = 0;
    
    // 检查value是否是JSON对象（以{开头）
    if (value[0] == '{') {
        // 如果value是JSON对象，直接嵌入到params中
        n = snprintf(payload, sizeof(payload),
                     "{\"id\":\"%lu\"\,\"params\":%s}",
                     (unsigned long)msg_id, value);
    } else {
        // 否则使用原来的单个key-value格式
        n = snprintf(payload, sizeof(payload),
                     "{\"id\":\"%lu\"\,\"params\":{\"%s\":{\"value\":%s}}}",
                     (unsigned long)msg_id, key, value);
    }
    // 调试打印 payload
    printf("MQTT Publish Data: Key=%s, Value=%s, Payload=%s\r\n", key, value, payload);
//    HAL_UART_Transmit(&huart2, (uint8_t*)"--PAYLOAD:", 10, 300);
//    if (n > 0) HAL_UART_Transmit(&huart2, (uint8_t*)payload, strlen(payload), 500);
//    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 100);
    
    if (n > 0 && n < (int)sizeof(payload))
    {
        // 尝试发布，只尝试一次，不重试避免阻塞
        if (ESP8266_MQTT_Publish(MQTT_TOPIC_POST, payload, 0, 0))
        {
            //HAL_UART_Transmit(&huart2, (uint8_t*)"Publish OK\r\n", 12, 100);
            msg_id++;
        }
        else
        {
           // HAL_UART_Transmit(&huart2, (uint8_t*)"Publish failed\r\n", 16, 100);
            // 不重试，避免阻塞主循环，下次再试
        }
    }
    else
    {
       // HAL_UART_Transmit(&huart2, (uint8_t*)"Payload build error\r\n", 21, 100);
    }
}


void MQTT_Publish_Dataw(float temperature,  float value)
{
    // 只在被 ISR 触发后或需要时执行

    uint32_t now = HAL_GetTick();
    // 非阻塞定时检查（可根据需要移除，如果 TIM4 已精准控制周期）
    if ((uint32_t)(now - last_pub) < PUB_INTERVAL_MS) return;
    last_pub = now;

    // 构造JSON payload
    char payload[256];
//    int n = snprintf(payload, sizeof(payload),
//                     "{\"id\":\"%lu\",\"params\":{\"temperature\":{\"value\":%.1f},\"humidity\":{\"value\":%.1f},\"led1\":{\"value\":%s},\\\"led2\":{\"value\":%s}}}",
//                     (unsigned long)msg_id, temperature, value, "true" , "true");
    int n = snprintf(payload, sizeof(payload),
                     "{\"id\":\"%lu\",\"params\":{\"temperature\":{\"value\":%.1f}}}",
                     (unsigned long)msg_id, temperature);
     // 调试打印 payload（便于确认发送内容）
//    HAL_UART_Transmit(&huart3, (uint8_t*)"--PAYLOAD:", 10, 300);
//    if (n > 0) HAL_UART_Transmit(&huart3, (uint8_t*)payload, strlen(payload), 500);
//    HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
    
    if (n > 0 && n < (int)sizeof(payload))
    {
        // 尝试发布，失败则重试并重连（在主循环中可以阻塞短时间）
        if (ESP8266_MQTT_Publish(MQTT_TOPIC_POST, payload, 0, 0))
        {
 //           HAL_UART_Transmit(&huart3, (uint8_t*)"Publish OK\r\n", 12, 100);
            msg_id++;
        }
        else
        {
//            HAL_UART_Transmit(&huart3, (uint8_t*)"Publish failed, retry...\r\n", 25, 100);
            delay_ms(200);

            if (!ESP8266_MQTT_Publish(MQTT_TOPIC_POST, payload, 0, 0))
            {
 //               HAL_UART_Transmit(&huart3, (uint8_t*)"Publish retry failed, reconnect MQTT\r\n", 35, 100);
                if (ESP8266_ConnectCloud())
								{
									 //                   HAL_UART_Transmit(&huart3, (uint8_t*)"MQTT reconnect OK\r\n", 19, 100);
								}
								
 
            }
            else
            {
                msg_id++;
            }
        }
    }
    else
    {
//        HAL_UART_Transmit(&huart3, (uint8_t*)"Payload build error\r\n", 21, 100);
    }
}
// 定时器4中断回调函数中调用发布函数
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    if (htim->Instance == TIM4)
//    {
//        MQTT_Publish_TriggerFromISR(); // 仅置标志
//    }
//}