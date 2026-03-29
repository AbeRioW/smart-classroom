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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "stdio.h"
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
uint16_t adc_value_in8 = 0;
uint16_t adc_value_in9 = 0;
uint8_t temp = 0, humi = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint16_t ADC_ReadChannel(uint32_t channel);
void Coarse_delay_us(uint32_t us);
static void DHT11_GPIO_MODE_SET(uint8_t mode);
void DHT11_START(void);
unsigned char DHT11_Check(void);
unsigned char DHT11_READ_BIT(void);
unsigned char DHT11_READ_BYTE(void);
uint8_t DHT11_ReadData(uint8_t *temp, uint8_t *humi);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  OLED_ShowString(0,0,(uint8_t*)"Initializing...",8,1);
  OLED_Refresh();
  HAL_Delay(1000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 读取ADC数据
    adc_value_in9 = ADC_ReadChannel(ADC_CHANNEL_9); // 光敏电阻
    adc_value_in8 = ADC_ReadChannel(ADC_CHANNEL_8); // MQ2
    
    // 清除OLED显示
    OLED_Clear();
    
    // 读取DHT11数据
    if (DHT11_ReadData(&temp, &humi) == 0) {
        // 显示温湿度数据
        char buffer[32];
        sprintf((char*)buffer, "Temp: %dC", temp);
        OLED_ShowString(0, 0, (uint8_t*)buffer, 8, 1);
        
        sprintf((char*)buffer, "Humi: %d%%", humi);
        OLED_ShowString(0, 8, (uint8_t*)buffer, 8, 1);
    } else {
        // 显示DHT11错误信息
        OLED_ShowString(0, 0, (uint8_t*)"DHT11 Error", 8, 1);
        OLED_ShowString(0, 8, (uint8_t*)"", 8, 1);
    }
    
    // 显示光敏电阻数据
    char buffer[32];
    sprintf((char*)buffer, "Light: %d", adc_value_in9);
    OLED_ShowString(0, 16, (uint8_t*)buffer, 8, 1);
    
    // 显示MQ2数据
    sprintf((char*)buffer, "MQ2: %d", adc_value_in8);
    OLED_ShowString(0, 24, (uint8_t*)buffer, 8, 1);
    
    // 刷新OLED显示
    OLED_Refresh();
    
    // 延时2秒
    HAL_Delay(2000);
  /* USER CODE END 3 */
}
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

/**
  * @brief  Reads ADC channel
  * @param  channel: ADC channel to read
  * @retval uint16_t: ADC conversion result
  */
uint16_t ADC_ReadChannel(uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  
  // Configure the ADC channel
  sConfig.Channel = channel;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
  
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  
  // Start ADC conversion
  HAL_ADC_Start(&hadc1);
  
  // Wait for conversion to complete
  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
  
  // Read conversion result
  uint16_t result = HAL_ADC_GetValue(&hadc1);
  
  // Stop ADC
  HAL_ADC_Stop(&hadc1);
  
  return result;
}

/**
  * @brief  Coarse delay in microseconds
  * @param  us: Delay time in microseconds
  * @retval None
  */
void Coarse_delay_us(uint32_t us)
{
  // Simple delay loop based on system clock
  // Adjust the divisor based on your actual clock speed
  uint32_t delay = (SystemCoreClock / 1000000) * us / 4;
  while (delay--)
  {
    __NOP();
  }
}

/**
  * @brief  Set DHT11 GPIO mode
  * @param  mode: 0 for input, 1 for output
  * @retval None
  */
static void DHT11_GPIO_MODE_SET(uint8_t mode)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  if (mode == 0) // Input mode
  {
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
  }
  else // Output mode
  {
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
  }
}

/**
  * @brief  Start DHT11 communication
  * @retval None
  */
void DHT11_START(void)
{
  DHT11_GPIO_MODE_SET(1); // Set to output
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET);
  Coarse_delay_us(18000); // 18ms delay
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);
  Coarse_delay_us(30); // 30us delay
  DHT11_GPIO_MODE_SET(0); // Set to input
}

/**
  * @brief  Check DHT11 response
  * @retval unsigned char: 0 for success, 1 for error
  */
unsigned char DHT11_Check(void)
{
  uint8_t retry = 0;
  
  // Wait for the pin to go low (response start)
  while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET && retry < 200)
  {
    retry++;
    Coarse_delay_us(1);
  }
  
  if (retry >= 200)
    return 1; // No response
  
  retry = 0;
  
  // Wait for the pin to go high (response acknowledge)
  while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_RESET && retry < 200)
  {
    retry++;
    Coarse_delay_us(1);
  }
  
  if (retry >= 200)
    return 1; // Acknowledge error
  
  retry = 0;
  
  // Wait for the pin to go low again (start of data transmission)
  while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET && retry < 200)
  {
    retry++;
    Coarse_delay_us(1);
  }
  
  if (retry >= 200)
    return 1; // Data start error
  
  return 0; // Response OK
}

/**
  * @brief  Read a bit from DHT11
  * @retval unsigned char: Read bit value
  */
unsigned char DHT11_READ_BIT(void)
{
  uint8_t retry = 0;
  
  // Wait for the pin to go high (start of bit)
  while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_RESET && retry < 100)
  {
    retry++;
    Coarse_delay_us(1);
  }
  
  if (retry >= 100)
    return 0; // Error, return 0
  
  // Wait for 40us to determine the bit value
  // According to DHT11 datasheet:
  // - '0' bit: high level lasts about 26-28us
  // - '1' bit: high level lasts about 70us
  Coarse_delay_us(40);
  
  // Read the pin state after 40us
  if (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET)
  {
    // It's a '1' bit, wait for it to finish
    while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET && retry < 100)
    {
      retry++;
      Coarse_delay_us(1);
    }
    return 1;
  }
  else
  {
    // It's a '0' bit
    return 0;
  }
}

/**
  * @brief  Read a byte from DHT11
  * @retval unsigned char: Read byte value
  */
unsigned char DHT11_READ_BYTE(void)
{
  unsigned char i, dat = 0;
  
  for (i = 0; i < 8; i++)
  {
    dat <<= 1;
    dat |= DHT11_READ_BIT();
  }
  
  return dat;
}

/**
  * @brief  Read data from DHT11
  * @param  temp: Pointer to temperature variable
  * @param  humi: Pointer to humidity variable
  * @retval uint8_t: 0 for success, 1 for error
  */
uint8_t DHT11_ReadData(uint8_t *temp, uint8_t *humi)
{
  unsigned char buf[5];
  uint8_t i;
  
  DHT11_START();
  
  if (DHT11_Check() == 0)
  {
    for (i = 0; i < 5; i++)
    {
      buf[i] = DHT11_READ_BYTE();
    }
    
    if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
    {
      *humi = buf[0];
      *temp = buf[2];
      return 0;
    }
    else
    {
      // 校验和错误
      return 1;
    }
  }
  else
  {
    // 响应错误
    return 1;
  }
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
