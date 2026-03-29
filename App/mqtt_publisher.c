#include "mqtt_publisher.h"
#include "delay.h"
#include "esp8266.h" // 包含ESP8266 MQTT相关函数
#include "usart.h"       // 包含UART相关函数
#include <stdio.h>
//#include <sys/_intsup.h>


// 模块内部变量
static uint32_t last_pub = 0;
#define PUB_INTERVAL_MS 3000 // 发布间隔，可根据需要修改

// 中断触发标志（volatile）
static volatile uint8_t mqtt_pub_flag = 0;

// 外部变量定义
uint32_t msg_id = 0;
int g_light_value = 0;

//DHT11_Data_t dht11;
int temp, humi;

// 在 ISR 中调用：仅置位标志
void MQTT_Publish_TriggerFromISR(void)
{
    mqtt_pub_flag = 1;
}

// 传感器数据读取与处理
static void Read_Sensors(void)
{
    // 读取光敏传感器并滤波
//    float raw = ADC_ReadLightSensor();
//    raw = (raw + ADC_ReadLightSensor() + ADC_ReadLightSensor()) / 3.0f;
//    g_light_value = ((4095 - raw) * 100) / 4095; // 0-100 转换

//    if(DHT11_ReadData(&dht11) == DHT11_OK)
//    {
//        temp = dht11.temp_int;
//        humi = dht11.humidity_int;
//    }
//    else
//    {
//        temp = 0; // 错误标志
//        humi = 0; // 错误标志
//    }
}

// MQTT数据发布实现（应在主循环/线程上下文调用，绝不可在 ISR 中调用）
void MQTT_Publish_Data(void)
{
    // 只在被 ISR 触发后或需要时执行
    if (!mqtt_pub_flag) return;
    mqtt_pub_flag = 0; // 清标志

    uint32_t now = HAL_GetTick();
    // 非阻塞定时检查（可根据需要移除，如果 TIM4 已精准控制周期）
    if ((uint32_t)(now - last_pub) < PUB_INTERVAL_MS) return;
    last_pub = now;

    Read_Sensors();

    HAL_GPIO_TogglePin(GPIOC,  GPIO_PIN_13); // 调试指示
    // 构造JSON payload
    char payload[256];
    int n = snprintf(payload, sizeof(payload),
                     "{\"id\":\"%lu\",\"params\":{\"light\":{\"value\":%d},\"Temp\":{\"value\":%d},\"Humi\":{\"value\":%d},\"led1\":{\"value\":%s},\"led2\":{\"value\":%s}}}",
                     (unsigned long)msg_id, g_light_value, temp, humi, "true" , "true");

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