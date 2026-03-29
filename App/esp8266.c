#include "esp8266.h"
#include "stm32f103xb.h"
#include "usart.h"
#include "delay.h"
#include "tim.h"
#include <stdint.h>
#include <stdio.h>

#include <string.h>
#include <stdarg.h>
#include "cJSON.h"
#include "gpio.h"
#include "stdlib.h"

        
// 全局变量定义
volatile uint8_t esp8266_buf[ESP8266_BUF_SIZE];          //接收缓冲区，存储 ESP8266 返回的数据
volatile uint16_t esp8266_cnt = 0, esp8266_cntPre = 0;   //缓冲区计数变量，用于判断接收是否完成
// uint8_t esp8266_rx_byte = 0;                    //中断接收的单个字节临时存储(可保留或删除，DMA 不用)

bool led1_status = 0; // LED1 状态变量
bool led2_status = 0; // LED2 状态变量

/* 等待缓冲中出现指定字符串（简单实现，单位 ms） */
static bool ESP8266_WaitForStr(const char *str, uint32_t timeout_ms)
{
    uint32_t ticks = timeout_ms / 10;
    while(ticks--)
    {
        delay_ms(10);
     // 快照长度并复制到本地缓冲，避免并发读取问题
        uint16_t len = esp8266_cnt;
        if (len)
        {
            if (len > ESP8266_BUF_SIZE - 1) len = ESP8266_BUF_SIZE - 1;
            char check_buf[ESP8266_BUF_SIZE];
            memcpy(check_buf, (const char*)esp8266_buf, len);
            check_buf[len] = '\0';
            if (str && strstr(check_buf, str) != NULL) return true;
        }
    }
    return false;
}

/**
*	函数名称：	ESP8266_Clear
*	函数功能：	清空接收缓存
*/
void ESP8266_Clear(void)
{
    memset((void *)esp8266_buf, 0, sizeof(esp8266_buf));
    esp8266_cnt = 0;
}

/**
*   函数名称：	ESP8266_WaitRecive
*	函数功能：	等待接收完成
*	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
*/
bool ESP8266_WaitRecive(void)
{
    if(esp8266_cnt == 0)
        return REV_WAIT;

    if(esp8266_cnt == esp8266_cntPre)
    {
        // 接收数据稳定，返回 OK（不要在这里清空 esp8266_cnt，避免竞态）
        esp8266_cntPre = 0;
        return REV_OK;
    }

    esp8266_cntPre = esp8266_cnt;
    return REV_WAIT;
}

/**
*	函数名称：	ESP8266_SendCmd
*	函数功能：	发送AT指令并等待响应
*	入口参数：	cmd：指令内容	expected_resp：期望的响应
*	返回参数：	true-成功	false-失败
*/
bool ESP8266_SendCmd(const char *cmd, const char *expected_resp)
{
  uint16_t timeOut = 1000; // 原 200 -> 1000 (约10s)

    // 清空接收缓冲区
    ESP8266_Clear();

    // 发送指令
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), 200);

    // 给模块一点时间开始回复（针对长命令适当延时）
    delay_ms(50);

    // 等待响应
    while(timeOut--)
    {
        if(ESP8266_WaitRecive() == REV_OK)
        {
            // 等待更多字节到达，避免分片丢失（可根据需要增大）
            delay_ms(200);

            // 拷贝缓冲到本地（用 esp8266_cnt 快照）
            char local_buf[ESP8266_BUF_SIZE];
            uint16_t len = esp8266_cnt;
            if (len > ESP8266_BUF_SIZE - 1) len = ESP8266_BUF_SIZE - 1;
            memcpy(local_buf, (const char*)esp8266_buf, len);
            local_buf[len] = '\0';

            // 调试输出完整接收内容
//            HAL_UART_Transmit(&huart3, (uint8_t*)"--ESP RX: ", 10, 100);
//            if (len) HAL_UART_Transmit(&huart3, (uint8_t*)local_buf, strlen(local_buf), 500);
//            HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);

            // 如果包含期望响应，返回成功
            if(expected_resp && strstr(local_buf, expected_resp) != NULL)
            {
                ESP8266_Clear();
                return true;
            }

            // 如果包含 ERROR，立即返回失败（并打印）
            if(strstr(local_buf, "ERROR") != NULL)
            {
                ESP8266_Clear();
                return false;
            }

            // 否则继续等待（可能是回显或其它异步信息）
        }
        delay_ms(10);
    }

      // 超时，拷贝并打印一次缓冲以便排查
    {
        char dump_buf[ESP8266_BUF_SIZE];
        uint16_t dump_len = esp8266_cnt;
        if (dump_len > ESP8266_BUF_SIZE - 1) dump_len = ESP8266_BUF_SIZE - 1;
        memcpy(dump_buf, (const char*)esp8266_buf, dump_len);
        dump_buf[dump_len] = '\0';

//        HAL_UART_Transmit(&huart3, (uint8_t*)"--ESP RX on timeout: ", 21, 100);
//        if (dump_len) HAL_UART_Transmit(&huart3, (uint8_t*)dump_buf, strlen(dump_buf), 500);
//        HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
    }

    ESP8266_Clear();
    return false;
}

/**
*	函数名称：	ESP8266_Reset
*	函数功能：	ESP8266硬件复位
*/
//void ESP8266_Reset(void){

//    // 拉低RST引脚复位
//    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);
//    delay_ms(100);
//    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET);
//    delay_ms(2000); // 等待模块重启
//}

/**
*	函数名称：	ESP8266_Init
*	函数功能：	ESP8266基础初始化（测试+模式+DHCP）
*/
void ESP8266_Init(void)
{

    //ESP8266_Reset(); // 硬件复位模块

    // 启动串口接收中断
    //HAL_UART_Receive_IT(&huart2, &esp8266_rx_byte, 1);
    // 启动串口 DMA 接收（替换原 HAL_UART_Receive_IT）
    HAL_UART_Receive_DMA(&huart1, (uint8_t*)esp8266_buf, ESP8266_BUF_SIZE - 1);
    
    // 测试AT指令
	 // HAL_UART_Transmit(&huart3, (uint8_t*)"AT test OK\r\n", 11, 100);
    if(ESP8266_SendCmd("AT\r\n", "OK"))
    { 
        //HAL_UART_Transmit(&huart3, (uint8_t*)"AT test OK\r\n", 11, 100);
        //OLED_PrintASCIIString(0, 0,"AT test OK", &afont8x6, OLED_COLOR_NORMAL);
        delay_ms(500);
    }else {
        //HAL_UART_Transmit(&huart3, (uint8_t*)"AT test failed, retrying...\r\n", 30, 100);
        //OLED_PrintASCIIString(0, 0,"AT test failed", &afont8x6, OLED_COLOR_NORMAL);
        delay_ms(500);
    }
  // HAL_UART_Transmit(&huart3, (uint8_t*)"AT test OK1\r\n", 11, 100);
    

    // 关闭回显，避免回显干扰后续匹配（ATE0）
    // 如果失败不致命，尝试多次但不死循环
    for (int i = 0; i < 3; ++i)
    {
        if (ESP8266_SendCmd("ATE0\r\n", "OK")) break;
        delay_ms(200);
    }


    // 设置Station模式
    if(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
    {    
       // HAL_UART_Transmit(&huart3, (uint8_t*)"CWMODE set to 1 OK\r\n", 19, 100);
        //OLED_PrintASCIIString(0, 10,"CWMODE set to 1", &afont8x6, OLED_COLOR_NORMAL);
        delay_ms(500);
    }else{
       // HAL_UART_Transmit(&huart3, (uint8_t*)"Set CWMODE failed, retrying...\r\n", 34, 100);
       // OLED_PrintASCIIString(0, 10,"Set CWMODE failed", &afont8x6, OLED_COLOR_NORMAL);
        delay_ms(500);
    }

    
    // 启用DHCP
    if(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
    {
        //HAL_UART_Transmit(&huart3, (uint8_t*)"DHCP enabled OK\r\n", 17, 100);
        //OLED_PrintASCIIString(0, 20,"DHCP enabled", &afont8x6, OLED_COLOR_NORMAL);
        delay_ms(500);
    }else{
       // HAL_UART_Transmit(&huart3, (uint8_t*)"Set DHCP failed, retrying...\r\n", 32, 100);
       // OLED_PrintASCIIString(0, 20,"Set DHCP failed", &afont8x6, OLED_COLOR_NORMAL);
        delay_ms(500);
    }
}

/**
 *	函数名称：	ESP8266_ConnectWiFi
 *	函数功能：	连接WiFi网络
 *	返回参数：	true-连接成功	false-连接失败
 */
bool ESP8266_ConnectWiFi(void)
{
    //const char *wifi_cmd = "AT+CWJAP=\"mhuan\",\"12345678\"\r\n";
    char wifi_cmd[128];  // 注意预留足够空间
    snprintf(wifi_cmd, sizeof(wifi_cmd), 
             "AT+CWJAP=\"%s\",\"%s\"\r\n", 
             WIFI_SSID, WIFI_PASSWORD); 
    
   // HAL_UART_Transmit(&huart3, (uint8_t*)"Connecting WiFi...\r\n", 19, 100);
   // OLED_PrintASCIIString(0, 0,"Connecting WiFi...", &afont8x6, OLED_COLOR_NORMAL);

    
    // 连接WiFi，等待GOT IP响应
    if(ESP8266_SendCmd(wifi_cmd, "GOT IP"))
    {
       // HAL_UART_Transmit(&huart3, (uint8_t*)"WiFi connected OK\r\n", 20, 100);
        //OLED_PrintASCIIString(0, 10,"WiFi connected", &afont8x6, OLED_COLOR_NORMAL);
        return true;
    }
    else
    {
       // HAL_UART_Transmit(&huart3, (uint8_t*)"WiFi connect failed\r\n", 22, 100);
       // OLED_PrintASCIIString(0, 10,"WiFi connect failed", &afont8x6, OLED_COLOR_NORMAL);
        return false;
    }
}

/**
 *	函数名称：	ESP8266_ConnectCloud
 *	函数功能：	连接OneNET云平台(MQTT)
 *	返回参数：	true-连接成功	false-连接失败
 */
bool ESP8266_ConnectCloud(void)
{
    //const char *mqtt_user_cfg = "AT+MQTTUSERCFG=0,1,\"rtos\",\"d9abErbb9x\",\"version=2018-10-31&res=products%2Fd9abErbb9x%2Fdevices%2Frtos&et=2052985695&method=md5&sign=vlPCi1NzGQgUTMexFN6qYA%3D%3D\",0,0,\"\"\r\n";
        char mqtt_user_cfg[512];  // 预留足够空间
    snprintf(mqtt_user_cfg, sizeof(mqtt_user_cfg),
             "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
             DEVICE_NAME,  // 设备名称（宏）
             PRODUCT_ID,   // 产品ID（宏）
             MQTT_TOKEN);       // 令牌（宏）

    const char *mqtt_conn = "AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n";
    
   // HAL_UART_Transmit(&huart3, (uint8_t*)"Configuring MQTT...\r\n", 22, 100);
       // OLED_PrintASCIIString(0, 20,"Configuring MQTT...", &afont8x6, OLED_COLOR_NORMAL);
    
    // 配置MQTT用户信息
    if(!ESP8266_SendCmd(mqtt_user_cfg, "OK"))
    {
			// HAL_UART_Transmit(&huart3, (uint8_t*)"MQTT USERCFG failed\r\n", 22, 100);
       // OLED_PrintASCIIString(0, 30,"MQTT USERCFG failed", &afont8x6, OLED_COLOR_NORMAL);
        return false;
    }
    
    //HAL_UART_Transmit(&huart3, (uint8_t*)"Connecting MQTT...\r\n", 20, 100);
    //OLED_PrintASCIIString(0, 40,"Connecting MQTT...", &afont8x6, OLED_COLOR_NORMAL);
    
    // 连接MQTT服务器
    if(ESP8266_SendCmd(mqtt_conn, "OK"))
    {
        //HAL_UART_Transmit(&huart3, (uint8_t*)"MQTT connected OK\r\n", 20, 100);
        //OLED_PrintASCIIString(0, 50,"MQTT connected", &afont8x6, OLED_COLOR_NORMAL);
        return true;
    }
    else
    {
       // HAL_UART_Transmit(&huart3, (uint8_t*)"MQTT connect failed\r\n", 22, 100);
        //OLED_PrintASCIIString(0, 50,"MQTT connect failed", &afont8x6, OLED_COLOR_NORMAL);
        return false;
    }
}

/**
 * 订阅 MQTT 主题
 * topic: 主题字符串，不带引号
 * qos: 0/1/2
 */
bool ESP8266_MQTT_Subscribe(const char *topic, uint8_t qos)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+MQTTSUB=0,\"%s\",%d\r\n", topic, qos);

    // 打印到调试串口，便于比对
   // HAL_UART_Transmit(&huart3, (uint8_t*)"--SEND CMD: ", 11, 100);
    //HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 500);
    //HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
    // 第一次尝试（允许更长等待）
    if (ESP8266_SendCmd(cmd, "OK"))
        return true;

    // 若失败，短延迟后重试一次
    delay_ms(200);
    return ESP8266_SendCmd(cmd, "OK");
}

/**
 * 发布 MQTT 消息
 * topic: 主题字符串，不带引号
 * payload: 要发送的 JSON 或文本
 * qos: 0/1/2
 * retain: 0/1
 */
/* 使用长度模式发布 MQTT（发送 AT+MQTTPUBRAW 带长度，等待 '>' 再发送 payload） */
bool ESP8266_MQTT_Publish(const char *topic, const char *payload, uint8_t qos, uint8_t retain)
{
    if(!topic || !payload) return false;

    uint32_t payload_len = strlen(payload);
    char cmd[256];

    // 构造带长度的命令（firmware 支持此格式时会返回 '>' 提示）
    // 格式：AT+MQTTPUB=0,"topic",<len>,<qos>,<retain>
    snprintf(cmd, sizeof(cmd), "AT+MQTTPUBRAW=0,\"%s\",%lu,%d,%d\r\n", topic, (unsigned long)payload_len, qos, retain);

    // 调试打印发送的命令
   // HAL_UART_Transmit(&huart3, (uint8_t*)"--SEND CMD:", 11, 100);
   // HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 500);
   // HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);

    // 清空接收缓存并发送命令
    ESP8266_Clear();
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 1000);

    // 等待 '>' 提示，给较长超时（例如 5s）
    if(!ESP8266_WaitForStr(">", 5000))
    {
        // 未收到 '>'，打印当前缓冲便于排查
        HAL_UART_Transmit(&huart1, (uint8_t*)"--ESP no prompt\r\n", 18, 100);
        return false;
    }

    // 发送原始 payload（不再加外层双引号）
    HAL_UART_Transmit(&huart1, (uint8_t*)payload, payload_len, 2000);

    // 发送完 payload 后，等待 OK 或 ERROR（最多 8s）
    if(ESP8266_WaitForStr("OK", 8000))
    {
        return true;
    }
    // 如果收到 ERROR 或超时则失败
    if(ESP8266_WaitForStr("ERROR", 100))
    {
        //HAL_UART_Transmit(&huart3, (uint8_t*)"--ESP publish ERROR\r\n", 22, 100);
    }
    else
    {
       // HAL_UART_Transmit(&huart3, (uint8_t*)"--ESP publish timeout\r\n", 23, 100);
    }
    return false;
}

/**
 * 检查接收缓冲区，寻找订阅主题的下发消息并解析 JSON 控制 LED
 * 说明：主循环中定期调用该函数（避免在中断中做复杂解析）
 */
void ESP8266_ProcessMessages(void)
{
    char local_buf[ESP8266_BUF_SIZE];
    uint16_t len = 0;
    len = (esp8266_cnt < ESP8266_BUF_SIZE - 1) ? esp8266_cnt : (ESP8266_BUF_SIZE - 1);
    if (len == 0) return;  
	printf("hello\r\n");
    memcpy(local_buf, (const char*)esp8266_buf, len);
    // 清空计数以表示已消费（由主线程负责）
    esp8266_cnt = 0;
    ((char*)esp8266_buf)[0] = '\0';
    local_buf[len] = '\0';

    // 调试打印收到的数据
    printf("--PROCESS RX: %s\r\n", local_buf);

    // 找到第一个 JSON 并提取完整 JSON（简单配对）
    char *p_json = strchr(local_buf, '{');
    if (!p_json) return;

    char json_buf[ESP8266_BUF_SIZE];
    char *p = p_json;
    int depth = 0;
    int idx = 0;
    while (*p && idx < (int)sizeof(json_buf) - 1)
    {
        if (*p == '{') depth++;
        if (*p == '}') depth--;
        json_buf[idx++] = *p++;
        if (depth == 0) break;
    }
    json_buf[idx] = '\0';
    if (idx == 0) return;

    // 解析 JSON
    cJSON *root = cJSON_Parse(json_buf);
    if (!root)
    {
       // HAL_UART_Transmit(&huart3, (uint8_t*)"cJSON parse failed\r\n", 20, 100);
        return;
    }

    // 取 id（用于回执）
    const char *id_str = NULL;
    cJSON *id_item = cJSON_GetObjectItem(root, "id");
    if (id_item && cJSON_IsString(id_item)) id_str = id_item->valuestring;

    // 解析 params
    // 支持同时处理多个 params 字段（led1, led2 等）
    cJSON *params = cJSON_GetObjectItem(root, "params");
    bool handled_any = false;
    if (params)
    {
        // 处理 led1（如果存在）
        cJSON *led1 = cJSON_GetObjectItem(params, "led1");
        if (led1)
        {
            int action = -1;
            if (cJSON_IsBool(led1)) action = cJSON_IsTrue(led1) ? 1 : 0;
            else if (cJSON_IsNumber(led1)) action = (int)cJSON_GetNumberValue(led1);
            else if (cJSON_IsString(led1)) action = atoi(led1->valuestring);
            else if (cJSON_IsObject(led1))
            {
                cJSON *v = cJSON_GetObjectItem(led1, "value");
                if (v)
                {
                    if (cJSON_IsBool(v)) action = cJSON_IsTrue(v) ? 1 : 0;
                    else if (cJSON_IsNumber(v)) action = (int)cJSON_GetNumberValue(v);
                    else if (cJSON_IsString(v)) action = atoi(v->valuestring);
                }
            }
            if (action >= 0)
            {
//                handled_any = true;
//                if (action) { LED1_Control(1); leds[0].state = true; HAL_UART_Transmit(&huart1, (uint8_t*)"LED1 ON\r\n", 9, 100); }
//                else       { LED1_Control(0); leds[0].state = false; HAL_UART_Transmit(&huart1, (uint8_t*)"LED1 OFF\r\n", 10, 100); }
            }
        }

        // 处理 led2（如果存在）
        cJSON *led2 = cJSON_GetObjectItem(params, "led2");
        if (led2)
        {
            int action = -1;
            if (cJSON_IsBool(led2)) action = cJSON_IsTrue(led2) ? 1 : 0;
            else if (cJSON_IsNumber(led2)) action = (int)cJSON_GetNumberValue(led2);
            else if (cJSON_IsString(led2)) action = atoi(led2->valuestring);
            else if (cJSON_IsObject(led2))
            {
                cJSON *v = cJSON_GetObjectItem(led2, "value");
                if (v)
                {
                    if (cJSON_IsBool(v)) action = cJSON_IsTrue(v) ? 1 : 0;
                    else if (cJSON_IsNumber(v)) action = (int)cJSON_GetNumberValue(v);
                    else if (cJSON_IsString(v)) action = atoi(v->valuestring);
                }
            }
            if (action >= 0)
            {
//                handled_any = true;
//                if (action) { LED2_Control(1); leds[1].state = true; HAL_UART_Transmit(&huart1, (uint8_t*)"LED2 ON\r\n", 9, 100); }
//                else       { LED2_Control(0); leds[1].state = false; HAL_UART_Transmit(&huart1, (uint8_t*)"LED2 OFF\r\n", 10, 100); }
            }
        }

        //处理更多 params 字段可按需添加
        
        // 处理 leds（亮度控制）
        cJSON *leds = cJSON_GetObjectItem(params, "leds");
        if (leds)
        {
            int brightness = -1;
            if (cJSON_IsNumber(leds)) brightness = (int)cJSON_GetNumberValue(leds);
            else if (cJSON_IsString(leds)) brightness = atoi(leds->valuestring);
            else if (cJSON_IsObject(leds))
            {
                cJSON *v = cJSON_GetObjectItem(leds, "value");
                if (v)
                {
                    if (cJSON_IsNumber(v)) brightness = (int)cJSON_GetNumberValue(v);
                    else if (cJSON_IsString(v)) brightness = atoi(v->valuestring);
                }
            }
            if (brightness >= 0 && brightness <= 2)
            {
                handled_any = true;
                // 调控PWM亮度
                uint32_t pwm_value = 0;
                switch(brightness)
                {
                    case 0: // 最暗
                        pwm_value = 0;
                        break;
                    case 1: // 中间亮度
                        pwm_value = 500;
                        break;
                    case 2: // 最高亮度
                        pwm_value = 1000;
                        break;
                }
                // 设置PWM占空比
                extern TIM_HandleTypeDef htim1;
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm_value);
                printf("LED brightness set to level %d (PWM: %lu)\r\n", brightness, (unsigned long)pwm_value);
                
                // 禁用自动调节，设置为手动模式
                extern volatile uint8_t brightness_level;
                brightness_level = brightness + 1; // 0→1, 1→2, 2→3
            }
        }

    }

 
    // 构造并发送回执 payload，使用接收到的 id（用于 set_reply）
        char reply_payload[128];
        if (id_str)
        {
            snprintf(reply_payload, sizeof(reply_payload),
                     "{\"id\":\"%s\",\"code\":200,\"data\":null,\"msg\":\"success\"}",
                     id_str);
        }
        else
        {
            snprintf(reply_payload, sizeof(reply_payload),
                     "{\"id\":\"%lu\",\"code\":200,\"data\":null,\"msg\":\"success\"}",
                     (unsigned long)HAL_GetTick());
        }

        // 使用已验证可用的 MQTTPUBRAW 发布回执（主题固定）
        ESP8266_MQTT_Publish(MQTT_TOPIC_SET_REPLY, reply_payload, 0, 0); 


    // 释放cJSON对象
    cJSON_Delete(root);

    // 若没有任何字段被处理，可直接返回
    if (!handled_any)
    {
        return;
    }



}


/**
*	函数名称：	HAL_UART_RxCpltCallback
*	函数功能：	串口接收中断回调函数（覆盖弱定义）
*	入口参数：	huart：串口句柄
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        // DMA 传输完整（缓冲已满），标记为满并重启接收
        esp8266_cnt = (ESP8266_BUF_SIZE - 1);
        esp8266_buf[esp8266_cnt] = '\0';
        HAL_UART_Receive_DMA(&huart1, (uint8_t*)esp8266_buf, ESP8266_BUF_SIZE - 1);
    }
}