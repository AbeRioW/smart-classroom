/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "delay.h"
#include "esp8266.h"
#include "dht11.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_flash_ex.h"
#include "mqtt_publisher.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 阈值数据结构体
typedef struct {
    uint16_t light_threshold;  // 光照阈值
    uint16_t mq2_threshold;    // MQ2阈值
    uint8_t temp_threshold;     // 温度阈值
    uint8_t hum_threshold;      // 湿度阈值
    uint32_t magic_number;      // 魔术数字，用于验证数据有效性
} Threshold_Data_t;

#define MAGIC_NUMBER 0x55AA55AA  // 魔术数字
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// FLASH存储地址，选择FLASH末尾的一页
#define FLASH_USER_START_ADDR   ((uint32_t)0x0800FC00)  // 对于64KB FLASH的STM32F103C8T6，最后一页的起始地址
#define FLASH_USER_END_ADDR     ((uint32_t)0x0800FFFF)  // FLASH结束地址
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 阈值变量
uint16_t light_threshold = 2000;  // 光照阈值，范围0-4200，步长500
uint16_t mq2_threshold = 2000;    // MQ2阈值，范围0-4200，步长500
uint8_t temp_threshold = 25;       // 温度阈值，范围0-40，步长1
uint8_t hum_threshold = 50;        // 湿度阈值，范围0-100，步长10

// 设置界面相关变量
uint8_t setting_mode = 0;  // 0: 正常模式，1: 设置模式
uint8_t setting_index = 0; // 0: 光照，1: MQ2，2: 温度，3: 湿度

// 舵机控制相关变量
uint8_t servo_flag = 0;  // 舵机控制标志，0: 空闲，1: 正在控制
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void TimeSettingInterface(void);
void SettingInterface(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void Flash_Read_Threshold(void);
void Flash_Write_Threshold(void);
void Servo_Control(uint16_t angle);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  从FLASH读取阈值数据
  * @retval None
  */
void Flash_Read_Threshold(void)
{
    Threshold_Data_t *threshold_data = (Threshold_Data_t *)FLASH_USER_START_ADDR;
    
    // 验证数据有效性
    if (threshold_data->magic_number == MAGIC_NUMBER)
    {
        // 数据有效，读取阈值
        light_threshold = threshold_data->light_threshold;
        mq2_threshold = threshold_data->mq2_threshold;
        temp_threshold = threshold_data->temp_threshold;
        hum_threshold = threshold_data->hum_threshold;
    }
    else
    {
        // 数据无效，使用默认值
        light_threshold = 4200;
        mq2_threshold = 4200;
        temp_threshold = 30;
        hum_threshold = 90;
        
        // 写入默认值到FLASH
        Flash_Write_Threshold();
    }
}

/**
  * @brief  控制SG90舵机转动到指定角度
  * @param  angle: 目标角度（0-180度）
  * @retval None
  */
void Servo_Control(uint16_t angle)
{
    // SG90舵机PWM信号周期为20ms，高电平时间为0.5ms-2.5ms对应0-180度
    // TIM2配置：72MHz / (7199+1) = 10kHz
    // 周期：199，PWM周期为 (199+1)/10kHz = 20ms，正确
    // 脉冲宽度：0.5ms-2.5ms对应比较值：5-25
    uint16_t compare_value = 5 + (angle * 20) / 18;  // 5 + (angle * 20)/18
    
    // 确保比较值在有效范围内
    if (compare_value < 5) compare_value = 5;    // 0度
    if (compare_value > 25) compare_value = 25;  // 180度
    
    // 设置PWM脉冲宽度
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare_value);
}

/**
  * @brief  向FLASH写入阈值数据
  * @retval None
  */
void Flash_Write_Threshold(void)
{
    HAL_FLASH_Unlock();  // 解锁FLASH
    
    // 擦除FLASH页
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
    EraseInitStruct.NbPages = 1;
    
    uint32_t PageError = 0;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    
    // 准备数据
    Threshold_Data_t threshold_data;
    threshold_data.light_threshold = light_threshold;
    threshold_data.mq2_threshold = mq2_threshold;
    threshold_data.temp_threshold = temp_threshold;
    threshold_data.hum_threshold = hum_threshold;
    threshold_data.magic_number = MAGIC_NUMBER;
    
    // 写入数据
    uint32_t *data_ptr = (uint32_t *)&threshold_data;
    for (uint8_t i = 0; i < sizeof(Threshold_Data_t) / 4; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_USER_START_ADDR + i * 4, data_ptr[i]);
    }
    
    HAL_FLASH_Lock();  // 锁定FLASH
}

/**
  * @brief  设置界面
  * @retval None
  */
void SettingInterface(void)
{
    while (setting_mode)
    {
        OLED_Clear();
        
        // 显示各个选项及其值，每个选项单独一行
        OLED_ShowString(0, 0, (uint8_t*)"Light:", 8, 1);
        OLED_ShowNum(40, 0, light_threshold, 4, 8, 1);
        
        OLED_ShowString(0, 8, (uint8_t*)"MQ2:", 8, 1);
        OLED_ShowNum(40, 8, mq2_threshold, 4, 8, 1);
        
        OLED_ShowString(0, 16, (uint8_t*)"Temp:", 8, 1);
        OLED_ShowNum(40, 16, temp_threshold, 2, 8, 1);
        OLED_ShowChar(55, 16, 'C', 8, 1);
        
        // 最后一个选项和EXIT在同一行
        OLED_ShowString(0, 24, (uint8_t*)"Hum:", 8, 1);
        OLED_ShowNum(40, 24, hum_threshold, 3, 8, 1);
        OLED_ShowChar(60, 24, '%', 8, 1);
        OLED_ShowString(80, 24, (uint8_t*)"EXIT", 8, 1);
        
        // 显示当前选中的选项
        switch (setting_index)
        {
            case 0:
                OLED_ShowChar(70, 0, '>', 8, 1);
                break;
            case 1:
                OLED_ShowChar(70, 8, '>', 8, 1);
                break;
            case 2:
                OLED_ShowChar(70, 16, '>', 8, 1);
                break;
            case 3:
                OLED_ShowChar(65, 24, '>', 8, 1);
                break;
        }
        
        OLED_Refresh();
        
        // 延迟一段时间，避免过于频繁的显示更新
        delay_ms(100);
    }
    
    // 退出设置模式时，保存阈值数据到FLASH
    Flash_Write_Threshold();
}

/**
  * @brief  按键中断回调函数
  * @param  GPIO_Pin: 触发中断的引脚
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 按键去抖
    delay_ms(10);
    
    if (GPIO_Pin == KEY1_Pin)
    {
        if (setting_mode)
        {
            // 在设置模式下，KEY1用于切换设置选项
            setting_index = (setting_index + 1) % 4;
            // 当索引回到0时，退出设置模式
            if (setting_index == 0)
            {
                setting_mode = 0;
            }
        }
        else
        {
            // 正常模式下，KEY1用于进入设置模式
            setting_mode = 1;
            setting_index = 0;
        }
    }
    else if (GPIO_Pin == KEY2_Pin && setting_mode)
    {
        // KEY2按下，在设置模式下增加当前选项的值
        switch (setting_index)
        {
            case 0: // 光照
                if (light_threshold < 4200)
                    light_threshold += 500;
                break;
            case 1: // MQ2
                if (mq2_threshold < 4200)
                    mq2_threshold += 500;
                break;
            case 2: // 温度
                if (temp_threshold < 40)
                    temp_threshold += 1;
                break;
            case 3: // 湿度
                if (hum_threshold < 100)
                    hum_threshold += 10;
                break;
        }
    }
    else if (GPIO_Pin == KEY3_Pin && setting_mode)
    {
        // KEY3按下，在设置模式下减少当前选项的值
        switch (setting_index)
        {
            case 0: // 光照
                if (light_threshold > 0)
                    light_threshold -= 500;
                break;
            case 1: // MQ2
                if (mq2_threshold > 0)
                    mq2_threshold -= 500;
                break;
            case 2: // 温度
                if (temp_threshold > 0)
                    temp_threshold -= 1;
                break;
            case 3: // 湿度
                if (hum_threshold > 0)
                    hum_threshold -= 10;
                break;
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  uint8_t wifi_try = 0, mqtt_try = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
    HAL_GPIO_WritePin(LAY_GPIO_Port, LAY_Pin, GPIO_PIN_RESET);
  Delay_Init();
  printf("go\r\n");
  
  // 从FLASH读取阈值数据
 // Flash_Read_Threshold();
  
  // 启动PWM
//  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 15); // 设置脉冲值为15，对应90度位置
//  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2); // 启动PWM输出
//  // 初始化舵机到90度
//  Servo_Control(90);
  
	#if 1
  while (wifi_try < 5 && !ESP8266_ConnectWiFi())
  {
		 printf("WiFi connect retry\r\n");
      wifi_try++;
      delay_ms(1000);
  }
	
				OLED_ShowString(0,0,(uint8_t*)"Cloud Connect...",8,1);
			OLED_Refresh();
	while(mqtt_try<20&&!ESP8266_ConnectCloud())
	{
		 printf("ConnectCloud retry\r\n");
      mqtt_try++;

      delay_ms(2000);
	}
	
	if(mqtt_try>20)
	{
							OLED_ShowString(0,0,(uint8_t*)"ConnectCloud failed",8,1);
			OLED_Refresh();
		  while(1);
	}
	 printf("ConnectCloud ok\r\n");

	if(!ESP8266_MQTT_Subscribe(MQTT_TOPIC_POST_REPLY,1))
	{
		 printf("MQTT subscribe post_reply failed\r\n");
		  OLED_Clear();
			OLED_ShowString(0,0,(uint8_t*)"ConnectCloud failed",8,1);
			OLED_Refresh();
		  while(1);
	}
	
		if(!ESP8266_MQTT_Subscribe(MQTT_TOPIC_SET,0))
	{
		  //HAL_UART_Transmit(&huart2, (uint8_t*)"MQTT subscribe failed\r\n", 22, 100);
		  while(1);
	}
	
	OLED_ShowString(0, 0, (uint8_t*)"init ok", 8, 1);
	OLED_Refresh();
	#endif
	            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500); // 设置脉冲值为500，对应0度
            delay_ms(1000); // 等待舵机到位
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 1500); // 设置脉冲值为1500，对应90度
            delay_ms(1000); // 等待舵机到位
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 500); // 设置脉冲值为500，对应0度
            delay_ms(1000); // 等待舵机到位
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if (setting_mode)
	  {
		  // 进入设置模式
		  SettingInterface();
	  }
	  else
	  {
		  // 读取ADC1和ADC2的数值
		  uint32_t adc1_value = ADC1_Read();
		  uint32_t adc2_value = ADC2_Read();
		  
		  // 显示调试信息
		  printf("Light value: %lu, threshold: %u, servo_flag: %d\r\n", adc1_value, light_threshold, servo_flag);
		  
		  // 检测光照值是否大于阈值
		  if (adc1_value > light_threshold && servo_flag == 0)
		  {
			  servo_flag = 1;  // 设置舵机控制标志
			  printf("Light value: %lu > threshold: %u, turning servo...\r\n", adc1_value, light_threshold);
			  
			  // 控制SG90舵机，完整转动一次
			  printf("Setting servo to 0 degrees...\r\n");
			  
			  // 延迟一段时间，避免重复触发
			  delay_ms(2000);
			  servo_flag = 0;  // 清除舵机控制标志
			  printf("Servo control finished, servo_flag reset to 0\r\n");
		  }
		  
		  // 读取DHT11温湿度数据
		  DHT11_Data_TypeDef dht11_data;
		  if (DHT11_Read_Data(&dht11_data) == 0)
		  {
			  // 在OLED上显示数值
			  OLED_Clear();
			  OLED_ShowString(0, 0, (uint8_t*)"ADC1(LDR): ", 8, 1);
			  OLED_ShowNum(70, 0, adc1_value, 4, 8, 1);
			  OLED_ShowString(0, 8, (uint8_t*)"ADC2(MQ2): ", 8, 1);
			  OLED_ShowNum(70, 8, adc2_value, 4, 8, 1);
			  OLED_ShowString(0, 16, (uint8_t*)"Temp:", 8, 1);
			  OLED_ShowNum(30, 16, dht11_data.temperature_int, 2, 8, 1);
			  OLED_ShowChar(45, 16, '.', 8, 1);
			  OLED_ShowNum(53, 16, dht11_data.temperature_dec, 1, 8, 1);
			  OLED_ShowChar(61, 16, 'C', 8, 1);
			  OLED_ShowString(0, 24, (uint8_t*)"Hum:", 8, 1);
			  OLED_ShowNum(30, 24, dht11_data.humidity_int, 2, 8, 1);
			  OLED_ShowChar(45, 24, '.', 8, 1);
			  OLED_ShowNum(53, 24, dht11_data.humidity_dec, 1, 8, 1);
			  OLED_ShowChar(61, 24, '%', 8, 1);
			  OLED_Refresh();
			  
			  // 将温度值发送到云端
			  char temp_str[16];
			  snprintf(temp_str, sizeof(temp_str), "%d.%d", dht11_data.temperature_int, dht11_data.temperature_dec);
			  MQTT_Publish_Data("temperature", temp_str);
			  printf("Temperature published to cloud: %s\r\n", temp_str);
				
			 char temp_str1[16];
			  snprintf(temp_str, sizeof(temp_str), "%d.%d", dht11_data.humidity_int, dht11_data.humidity_dec);
			  MQTT_Publish_Data("humidity", temp_str);
			  printf("Temperature published to cloud: %s\r\n", temp_str);
				
							 char temp_str2[16];
			  snprintf(temp_str, sizeof(temp_str), "%d", adc2_value);
			  MQTT_Publish_Data("MQ2", temp_str);
			  printf("Temperature published to cloud: %s\r\n", temp_str);
				
											 char temp_str3[16];
			  snprintf(temp_str, sizeof(temp_str), "%d", adc1_value);
			  MQTT_Publish_Data("light", temp_str);
			  printf("Temperature published to cloud: %s\r\n", temp_str);
			  
			  // 检测温度和湿度是否超过阈值
			  if (dht11_data.humidity_int > hum_threshold)
			  {
				  // 温湿度超过阈值，拉高继电器
				  //HAL_GPIO_WritePin(LAY_GPIO_Port, LAY_Pin, GPIO_PIN_SET);
				  printf("Temperature or humidity exceeds threshold, relay ON\r\n");
			  }
			  else
			  {
				  // 温湿度在正常范围内，关闭继电器
				  //HAL_GPIO_WritePin(LAY_GPIO_Port, LAY_Pin, GPIO_PIN_RESET);
				  printf("Temperature and humidity normal, relay OFF\r\n");
			  }
		  }
		  
		  // 检测MQ2值是否大于阈值
		  static uint8_t beep_flag = 0;
		  static uint32_t beep_start_time = 0;
		  if (adc2_value > mq2_threshold && beep_flag == 0)
		  {
			  beep_flag = 1;  // 设置蜂鸣器控制标志
			  beep_start_time = HAL_GetTick();  // 记录开始时间
			  printf("MQ2 value: %lu > threshold: %u, activating beep...\r\n", adc2_value, mq2_threshold);
			  
			  // 拉低BEEP进行报警
			  HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
		  }
		  
		  // 检查蜂鸣器是否需要关闭
		  if (beep_flag == 1 && (HAL_GetTick() - beep_start_time) >= 5000)
		  {
			  // 关闭蜂鸣器
			  HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
			  printf("Beep deactivated after 5 seconds\r\n");
			  
			  // 清除蜂鸣器控制标志
			  beep_flag = 0;
		  }
		  
		  // 延迟一段时间
		  delay_ms(1000);
	  }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
