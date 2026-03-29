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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

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
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void TimeSettingInterface(void);
void SettingInterface(void);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  设置界面
  * @retval None
  */
void SettingInterface(void)
{
    while (setting_mode)
    {
        OLED_Clear();
        
        // 显示标题
        OLED_ShowString(0, 0, (uint8_t*)"Settings", 8, 1);
        
        // 显示各个选项及其值
        OLED_ShowString(0, 8, (uint8_t*)"Light: ", 8, 1);
        OLED_ShowNum(40, 8, light_threshold, 4, 8, 1);
        
        OLED_ShowString(0, 16, (uint8_t*)"MQ2:   ", 8, 1);
        OLED_ShowNum(40, 16, mq2_threshold, 4, 8, 1);
        
        OLED_ShowString(0, 24, (uint8_t*)"Temp:  ", 8, 1);
        OLED_ShowNum(40, 24, temp_threshold, 2, 8, 1);
        OLED_ShowChar(55, 24, 'C', 8, 1);
        
        // 显示当前选中的选项
        switch (setting_index)
        {
            case 0:
                OLED_ShowChar(60, 8, '>', 8, 1);
                break;
            case 1:
                OLED_ShowChar(60, 16, '>', 8, 1);
                break;
            case 2:
                OLED_ShowChar(60, 24, '>', 8, 1);
                break;
            case 3:
                OLED_ShowString(0, 24, (uint8_t*)"Hum:   ", 8, 1);
                OLED_ShowNum(40, 24, hum_threshold, 3, 8, 1);
                OLED_ShowChar(58, 24, '%', 8, 1);
                OLED_ShowChar(65, 24, '>', 8, 1);
                break;
        }
        
        // 显示退出提示
        OLED_ShowString(80, 24, (uint8_t*)"EXIT", 8, 1);
        
        OLED_Refresh();
        
        // 延迟一段时间，避免过于频繁的显示更新
        delay_ms(100);
    }
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
  MX_TIM1_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  
  Delay_Init();
  printf("go\r\n");
  
	#if 0
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
