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
#include "HC_SR04.h"
#include "delay.h"
#include "esp8266.h"
#include "ds1302.h"
#include "adc.h"
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
extern volatile uint8_t timer1_second_flag;
extern volatile uint16_t timer1_counter;

// Brightness control variables
volatile uint8_t brightness_level = 0; // 0: auto, 1: low, 2: medium, 3: high
volatile uint8_t key1_pressed = 0;
volatile uint8_t key2_pressed = 0;
volatile uint8_t key3_pressed = 0;
volatile uint16_t key1_debounce = 0;
volatile uint16_t key2_debounce = 0;
volatile uint16_t key3_debounce = 0;
const uint16_t DEBOUNCE_TIME = 200; // 200ms debounce time
const uint32_t brightness_values[] = {0, 333, 666, 1000}; // 0: auto, 1: 33%, 2: 66%, 3: 100%

// Time setting variables
volatile uint8_t time_setting_mode = 0; // 0: normal mode, 1: time setting mode

// Ultrasonic threshold variable
volatile uint8_t ultrasonic_threshold = 5; // Default value (0-10)

// HCSR505 and Beep control variables
volatile uint8_t hcsr505_high_count = 0; // Counter for consecutive HCSR505 high level detections
volatile uint8_t beep_active = 0; // Flag indicating if beep is active
volatile uint32_t beep_start_time = 0; // Timestamp when beep was activated
const uint8_t HCSR505_REQUIRED_COUNT = 5; // Required consecutive high level detections
const uint32_t BEEP_DURATION = 5000; // Beep duration in milliseconds (5 seconds)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void TimeSettingInterface(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// GPIO EXTI callback function
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == KEY1_Pin)
    {
        if(key1_debounce == 0)
        {
            if(time_setting_mode == 0)
            {
                // KEY1 pressed: enter time setting mode
                time_setting_mode = 1;
            }
            else
            {
                // KEY1 pressed in setting mode: decrease current setting value
                key1_pressed = 1;
            }
            
            // Set debounce time
            key1_debounce = DEBOUNCE_TIME;
        }
    }
    else if(GPIO_Pin == KEY2_Pin)
    {
        if(key2_debounce == 0)
        {
            // KEY2 pressed: cycle through brightness levels or adjust setting value
            if(time_setting_mode == 0)
            {
                // Normal mode: adjust brightness
                brightness_level++;
                if(brightness_level > 3)
                {
                    brightness_level = 1; // Reset to low after high
                }
                
                // Set manual brightness
                if(brightness_level > 0)
                {
                    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, brightness_values[brightness_level]);
                }
            }
            else
            {
                // Setting mode: increase current setting value
                key2_pressed = 1;
            }
            
            // Set debounce time
            key2_debounce = DEBOUNCE_TIME;
        }
    }
    else if(GPIO_Pin == KEY3_Pin)
    {
        if(key3_debounce == 0)
        {
            // KEY3 pressed: return to auto mode or switch setting item
            if(time_setting_mode == 0)
            {
                // Normal mode: return to auto brightness
                brightness_level = 0;
            }
            else
            {
                // Setting mode: switch to next setting item
                key3_pressed = 1;
            }
            
            // Set debounce time
            key3_debounce = DEBOUNCE_TIME;
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
  //MX_TIM1_Init();
  //MX_ADC1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
 // OLED_Init();
 // HC_SR04_Init();
 // DS1302_Init();
  
  Delay_Init();
  
  // 启动TIM1
  //HAL_TIM_Base_Start_IT(&htim1);
  // 启动TIM1 PWM
  //HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  
  // 初始设置时间（2026-03-23 12:00:00）
//  DS1302_Time init_time;
//  init_time.year = 26;
//  init_time.month = 3;
//  init_time.date = 23;
//  init_time.day = 6; // 0=Sunday, 6=Saturday
//  init_time.hour = 12;
//  init_time.minute = 0;
//  init_time.second = 0;
//  DS1302_SetTime(&init_time);
  
	#if 1
  while (wifi_try < 5 && !ESP8266_ConnectWiFi())
  {
		 printf("WiFi connect retry\r\n");
      wifi_try++;
      HAL_Delay(1000);
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
	
	#endif
	
	OLED_Clear();
	OLED_ShowString(0,0,(uint8_t*)"Distance:",8,1);
	OLED_ShowString(0,8,(uint8_t*)"Time:",8,1);
	OLED_ShowString(0,16,(uint8_t*)"Date:",8,1);
	OLED_Refresh();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		ESP8266_ProcessMessages();
    // Check if in time setting mode
    if(time_setting_mode)
    {
        TimeSettingInterface();
    }
    else
    {
	//printf("go\r\n");
    float distance = HC_SR04_MeasureDistance();
    if(distance >= 0)
    {
        uint32_t int_part = (uint32_t)distance;
        uint32_t decimal_part = (uint32_t)((distance - int_part) * 100);
        OLED_ShowNum(70,0,int_part,3,8,1);
        OLED_ShowChar(94,0,'.',8,1);
        OLED_ShowNum(102,0,decimal_part,2,8,1);
        OLED_ShowString(110,0,(uint8_t*)"cm",8,1);
        
        // 刷新距离显示
        OLED_Refresh();
    }
    else
    {
        //OLED_ShowString(70,0,(uint8_t*)"Err",8,1);
    }
    
    // HCSR505 and Beep control logic
    uint8_t hcsr505_state = HAL_GPIO_ReadPin(HC_SR505_GPIO_Port, HC_SR505_Pin);
    
    if(hcsr505_state == GPIO_PIN_SET)
    {
        hcsr505_high_count++;
        if(hcsr505_high_count > HCSR505_REQUIRED_COUNT)
        {
            hcsr505_high_count = HCSR505_REQUIRED_COUNT;
        }
    }
    else
    {
        hcsr505_high_count = 0;
    }
    
    // Check if beep duration has elapsed
    if(beep_active)
    {
        if(HAL_GetTick() - beep_start_time >= BEEP_DURATION)
        {
            HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
            beep_active = 0;
        }
    }
    
    // Activate beep if conditions are met
    if(hcsr505_high_count >= HCSR505_REQUIRED_COUNT && distance >= 0 && distance < ultrasonic_threshold && !beep_active)
    {
        HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
        beep_active = 1;
        beep_start_time = HAL_GetTick();
    }
    
    // 读取ADC值（光照值）- 仅在自动模式下
    if(brightness_level == 0)
    {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
        {
            uint32_t adc_value = HAL_ADC_GetValue(&hadc1);
            
            // 根据光照值计算PWM占空比
            // 光照值越大，LED越暗；光照值越小，LED越亮
            uint32_t pwm_value = 1000 - (adc_value * 1000 / 4095);
            if (pwm_value > 1000) pwm_value = 1000;
            if (pwm_value < 0) pwm_value = 0;
            
            // 设置PWM占空比
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pwm_value);
        }
        HAL_ADC_Stop(&hadc1);
    }
    
    // 1秒显示一次时间
    if(timer1_second_flag)
    {
      timer1_second_flag = 0;
      DS1302_Time time;
      DS1302_GetTime(&time);
      
      // 显示时间：HH:MM:SS (第二行，位置8)
      OLED_ShowNum(40,8,time.hour,2,8,1);
      OLED_ShowChar(56,8,':',8,1);
      OLED_ShowNum(64,8,time.minute,2,8,1);
      OLED_ShowChar(80,8,':',8,1);
      OLED_ShowNum(88,8,time.second,2,8,1);
      
      // 显示日期：YYYY-MM-DD (第三行，位置16)
      OLED_ShowNum(40,16,2000 + time.year,4,8,1);
      OLED_ShowChar(72,16,'-',8,1);
      OLED_ShowNum(80,16,time.month,2,8,1);
      OLED_ShowChar(96,16,'-',8,1);
      OLED_ShowNum(104,16,time.date,2,8,1);
      
      // 只有在更新时间时才刷新OLED显示
      OLED_Refresh();
    }
    
    // 保持主循环运行，不使用delay_ms阻塞
    delay_ms(100);
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

// Time setting interface function
void TimeSettingInterface(void)
{
    // Time structure for setting
    DS1302_Time setting_time;
    DS1302_GetTime(&setting_time);
    
    // Use global ultrasonic threshold variable
    
    // Current setting item (0: year, 1: month, 2: date, 3: hour, 4: minute, 5: second, 6: ultrasonic threshold, 7: exit)
    uint8_t current_item = 0;
    uint8_t previous_item = 0;
    uint8_t setting_active = 1;
    
    // Clear OLED display once at the beginning
    OLED_Clear();
    
    // Display static labels once
    OLED_ShowString(0, 0, (uint8_t*)"Year:", 8, 1);
    OLED_ShowString(0, 8, (uint8_t*)"Month:", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"Date:", 8, 1);
    OLED_ShowString(0, 24, (uint8_t*)"Hour:", 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"Minute:", 8, 1);
    OLED_ShowString(0, 40, (uint8_t*)"Second:", 8, 1);
    OLED_ShowString(0, 48, (uint8_t*)"Threshold:", 8, 1);
    OLED_ShowString(0, 56, (uint8_t*)"Exit", 8, 1);
    
    // Initial display of values
    OLED_ShowNum(40, 0, 2000 + setting_time.year, 4, 8, 1);
    OLED_ShowNum(48, 8, setting_time.month, 2, 8, 1);
    OLED_ShowNum(40, 16, setting_time.date, 2, 8, 1);
    OLED_ShowNum(40, 24, setting_time.hour, 2, 8, 1);
    OLED_ShowNum(56, 32, setting_time.minute, 2, 8, 1);
    OLED_ShowNum(56, 40, setting_time.second, 2, 8, 1);
    OLED_ShowNum(72, 48, ultrasonic_threshold, 2, 8, 1);
    
    // Show initial highlight
    OLED_ShowString(110, 0, (uint8_t*)">", 8, 1);
    OLED_Refresh();
    
    while(setting_active)
    {
        // Clear previous highlight if item has changed
        if(current_item != previous_item)
        {
            // Clear previous highlight by displaying space
            switch(previous_item)
            {
                case 0: // Year
                    OLED_ShowString(110, 0, (uint8_t*)" ", 8, 1);
                    break;
                case 1: // Month
                    OLED_ShowString(110, 8, (uint8_t*)" ", 8, 1);
                    break;
                case 2: // Date
                    OLED_ShowString(110, 16, (uint8_t*)" ", 8, 1);
                    break;
                case 3: // Hour
                    OLED_ShowString(110, 24, (uint8_t*)" ", 8, 1);
                    break;
                case 4: // Minute
                    OLED_ShowString(110, 32, (uint8_t*)" ", 8, 1);
                    break;
                case 5: // Second
                    OLED_ShowString(110, 40, (uint8_t*)" ", 8, 1);
                    break;
                case 6: // Ultrasonic threshold
                    OLED_ShowString(110, 48, (uint8_t*)" ", 8, 1);
                    break;
                case 7: // Exit
                    OLED_ShowString(110, 56, (uint8_t*)" ", 8, 1);
                    break;
            }
            
            // Show current highlight
            switch(current_item)
            {
                case 0: // Year
                    OLED_ShowString(110, 0, (uint8_t*)">", 8, 1);
                    break;
                case 1: // Month
                    OLED_ShowString(110, 8, (uint8_t*)">", 8, 1);
                    break;
                case 2: // Date
                    OLED_ShowString(110, 16, (uint8_t*)">", 8, 1);
                    break;
                case 3: // Hour
                    OLED_ShowString(110, 24, (uint8_t*)">", 8, 1);
                    break;
                case 4: // Minute
                    OLED_ShowString(110, 32, (uint8_t*)">", 8, 1);
                    break;
                case 5: // Second
                    OLED_ShowString(110, 40, (uint8_t*)">", 8, 1);
                    break;
                case 6: // Ultrasonic threshold
                    OLED_ShowString(110, 48, (uint8_t*)">", 8, 1);
                    break;
                case 7: // Exit
                    OLED_ShowString(110, 56, (uint8_t*)">", 8, 1);
                    break;
            }
            
            // Update previous item
            previous_item = current_item;
            
            // Refresh display after highlight change
            OLED_Refresh();
        }
        
        // Update values if they have changed
        OLED_ShowNum(40, 0, 2000 + setting_time.year, 4, 8, 1);
        OLED_ShowNum(48, 8, setting_time.month, 2, 8, 1);
        OLED_ShowNum(40, 16, setting_time.date, 2, 8, 1);
        OLED_ShowNum(40, 24, setting_time.hour, 2, 8, 1);
        OLED_ShowNum(56, 32, setting_time.minute, 2, 8, 1);
        OLED_ShowNum(56, 40, setting_time.second, 2, 8, 1);
        OLED_ShowNum(72, 48, ultrasonic_threshold, 2, 8, 1);
        
        // Refresh display for value updates
        OLED_Refresh();
        
        // Add a small delay to reduce flickering
        delay_ms(50);
        
        // Check for key presses
        if(key1_pressed)
        {
            key1_pressed = 0;
            
            switch(current_item)
            {
                case 0: // Year
                    setting_time.year--;
                    if(setting_time.year < 0) setting_time.year = 99;
                    break;
                case 1: // Month
                    setting_time.month--;
                    if(setting_time.month < 1) setting_time.month = 12;
                    break;
                case 2: // Date
                    setting_time.date--;
                    if(setting_time.date < 1)
                    {
                        // Simple date validation (not perfect, but works for most cases)
                        uint8_t max_days = 31;
                        if(setting_time.month == 4 || setting_time.month == 6 || setting_time.month == 9 || setting_time.month == 11)
                            max_days = 30;
                        else if(setting_time.month == 2)
                            max_days = 28; // Simplified, not considering leap years
                        setting_time.date = max_days;
                    }
                    break;
                case 3: // Hour
                    setting_time.hour--;
                    if(setting_time.hour < 0) setting_time.hour = 23;
                    break;
                case 4: // Minute
                    setting_time.minute--;
                    if(setting_time.minute < 0) setting_time.minute = 59;
                    break;
                case 5: // Second
                    setting_time.second--;
                    if(setting_time.second < 0) setting_time.second = 59;
                    break;
                case 6: // Ultrasonic threshold
                    ultrasonic_threshold--;
                    if(ultrasonic_threshold < 0) ultrasonic_threshold = 10;
                    break;
                case 7: // Exit
                    // Save the settings
                    DS1302_SetTime(&setting_time);
                    setting_active = 0;
                    time_setting_mode = 0;
                    break;
            }
        }
        
        if(key2_pressed)
        {
            key2_pressed = 0;
            
            switch(current_item)
            {
                case 0: // Year
                    setting_time.year++;
                    if(setting_time.year > 99) setting_time.year = 0;
                    break;
                case 1: // Month
                    setting_time.month++;
                    if(setting_time.month > 12) setting_time.month = 1;
                    break;
                case 2: // Date
                    setting_time.date++;
                    // Simple date validation (not perfect, but works for most cases)
                    uint8_t max_days = 31;
                    if(setting_time.month == 4 || setting_time.month == 6 || setting_time.month == 9 || setting_time.month == 11)
                        max_days = 30;
                    else if(setting_time.month == 2)
                        max_days = 28; // Simplified, not considering leap years
                    if(setting_time.date > max_days) setting_time.date = 1;
                    break;
                case 3: // Hour
                    setting_time.hour++;
                    if(setting_time.hour > 23) setting_time.hour = 0;
                    break;
                case 4: // Minute
                    setting_time.minute++;
                    if(setting_time.minute > 59) setting_time.minute = 0;
                    break;
                case 5: // Second
                    setting_time.second++;
                    if(setting_time.second > 59) setting_time.second = 0;
                    break;
                case 6: // Ultrasonic threshold
                    ultrasonic_threshold++;
                    if(ultrasonic_threshold > 10) ultrasonic_threshold = 0;
                    break;
                case 7: // Exit
                    // Save the settings
                    DS1302_SetTime(&setting_time);
                    setting_active = 0;
                    time_setting_mode = 0;
                    break;
            }
        }
        
        if(key3_pressed)
        {
            key3_pressed = 0;
            current_item++;
            if(current_item > 7) current_item = 0;
        }
        
        // Small delay to avoid rapid changes (already added earlier)
        // delay_ms(100);
    }
    
    // Clear display and return to normal mode
    OLED_Clear();
    OLED_ShowString(0,0,(uint8_t*)"Distance:",8,1);
    OLED_ShowString(0,8,(uint8_t*)"Time:",8,1);
    OLED_ShowString(0,16,(uint8_t*)"Date:",8,1);
    OLED_Refresh();
}

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
