/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#include "vl53l5cx_api.h"
#include "vl53l5cx.h"
#include "platform.h"
#include "stm32f4xx_nucleo_bus.h"
#include "custom_tof_conf.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MAX_ANGLE 1653
#define MIN_ANGLE 1398
#define ANGLE_BEGIN 1525
#define MAX_SPEED 7000
#define MIN_SPEED 3000

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//#ifdef __GNUC__
//#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
//#else
//#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
//
//#endif
//
//PUTCHAR_PROTOTYPE
//{
//	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
//	return ch;
//}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
int CheckSum(uint8_t* data);
void parseString(const uint8_t* data, int len, uint8_t delim);
void changeAngle(uint8_t angle);
void changeSpeed(uint8_t speed, uint8_t direction);
int searchKeyword(const uint8_t* data, uint8_t* pat, int m, int n);
uint8_t initSensor(VL53L5CX_Configuration* conf);
void U16_2_U8(const int16_t src, uint8_t* upper, uint8_t* lower);
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
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  BSP_I2C1_Init();

  /* USER CODE BEGIN 2 */

  VL53L5CX_Platform plat;
  plat.GetTick = (VL53L5CX_get_tick_Func) BSP_GetTick;
  plat.Read = (VL53L5CX_read_Func) CUSTOM_VL53L5CX_I2C_ReadReg ;
  plat.Write = (VL53L5CX_write_Func) CUSTOM_VL53L5CX_I2C_WriteReg;
  VL53L5CX_Configuration conf;
  conf.platform = plat;
  initSensor(&conf);

  // start PWM
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);

  uint16_t angle = ANGLE_BEGIN;
  uint16_t speed = MIN_SPEED;

  // + angle: left, - angle: right

  // set to beginning speed and angle
  htim4.Instance->CCR1 = angle;
  htim3.Instance->CCR2 = speed;

  // start PWM
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);

  uint8_t buffer[8]; // command buffer
  uint16_t bufSize = 8 * sizeof(uint8_t); // buffer size
  uint8_t delim = '\r'; // delimiter
  uint8_t preamble[6] = {'s', 't', 'a', 'r', 't', '\n'}; // preamble
  uint8_t stop[5] = {'s', 't', 'o', 'p', '\n'}; // stop condition
//  uint8_t ack[4] = {'g', 'e', 't', '\n'};

//  char buf[10];

  int start = 0;
  uint8_t ready = 0;
  VL53L5CX_ResultsData data;
  uint8_t rangeData[130];
  memset(rangeData, 0, 130*sizeof(uint8_t));
  rangeData[0] = '\r';
  rangeData[129] = '\r';
  uint8_t lower, upper;
  uint16_t pSize = 130 * sizeof(uint8_t);
//  vl53l5cx_start_ranging(&conf);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	while (!start) {
		int ret = 0;
		HAL_UART_Receive(&huart1, buffer, 8, 100);
		ret = searchKeyword(buffer, preamble, 8, 6);
		if (ret) {
//			HAL_Delay(2000);
//			HAL_UART_Transmit(&huart1, ack, 4, 100);
			HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
			HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
			start = 1;
			vl53l5cx_start_ranging(&conf);
		}
	}

	while (start) {
		int ret = 0;
		HAL_UART_Receive(&huart1, buffer, 8, 100);
		parseString(buffer, 8, delim);
		ret = searchKeyword(buffer, stop, 8, 5);

		// check for ranging data
		while (!ready) {
			vl53l5cx_check_data_ready(&conf, &ready);
			HAL_Delay(100);
		}
		// get ranging data
		vl53l5cx_get_ranging_data(&conf, &data);

		// convert data to unsigned character
		for (int i=1; i<65; i++) {
			U16_2_U8(data.distance_mm[i-1], &upper, &lower);
			rangeData[i] = lower;
			rangeData[i+64] = upper;
		}

		// send ranging data
		HAL_UART_Transmit(&huart1, rangeData,
				pSize, 100);
		ready = 0;

		if (ret) {
//			HAL_Delay(2000);
//			HAL_UART_Transmit(&huart1, ack, 4, 100);
			HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
			HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
			start = 0;
			vl53l5cx_stop_ranging(&conf);
		}
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 84;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 20000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 84;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 20000-1;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_10|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB10 PB4 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_10|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

int CheckSum(uint8_t* data) {
	uint8_t checksum = data[3]; // byte after data is checksum
	uint8_t encoded = data[0] | data[1] | data[2]; // checksum should be bitwise of data
	if (encoded == checksum) {
		return 1;
	}
	return 0;
}

void parseString(const uint8_t* data, int len, uint8_t delim) {
	uint8_t currData[5] = {0}; // command data
	char buf[10];
	int l = len - 5 + 1;
	for (int i=0; i<l; i++) {
		memcpy(currData, &data[i], 5);
		if (CheckSum(currData) && currData[4] == delim) {
			changeSpeed(currData[1], currData[2]);
			changeAngle(currData[0]);
			sprintf(buf, "%d,%d,%d\n\r", currData[0],
					currData[1], currData[2]);
			HAL_UART_Transmit(&huart2, buf,
					sizeof(char)*10, 100);
			break;
		}
	}
}

void changeAngle(uint8_t angle) {
	uint16_t deg = MIN_ANGLE + angle;
	htim4.Instance->CCR1 = deg;
//	HAL_Delay(200);
	return;
}

void changeSpeed(uint8_t speed, uint8_t direction) {
	uint16_t mapped = (uint16_t)(speed) << 4;
	uint16_t vel  = MIN_SPEED + mapped;
	vel %= MAX_SPEED;
	htim3.Instance->CCR2 = vel;

	if (direction) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
	}
	else {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
	}
//	HAL_Delay(500);
}

int searchKeyword(const uint8_t* data, uint8_t* pat, int m, int n) {
	int k = 0;
	for (int i=0; i<m; i++) {
		// check for matching character
		if (data[i] == pat[k]) {
			k++;
			if (k == n) {
				return 1;
			}
		}
		// if unmatched return match to idx 0
		else {
			k = 0;
		}
	}
	return 0;
}

uint8_t initSensor(VL53L5CX_Configuration* conf) {
	uint8_t isAlive = 0;
	uint8_t status = 0;
	conf->platform.address = VL53L5CX_DEFAULT_I2C_ADDRESS;
	HAL_GPIO_WritePin(CUSTOM_VL53L5CX_I2C_RST_PORT, CUSTOM_VL53L5CX_I2C_RST_PIN, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(CUSTOM_VL53L5CX_PWR_EN_PORT, CUSTOM_VL53L5CX_PWR_EN_PIN , GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(CUSTOM_VL53L5CX_LPn_PORT, CUSTOM_VL53L5CX_LPn_PIN, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(CUSTOM_VL53L5CX_PWR_EN_PORT, CUSTOM_VL53L5CX_PWR_EN_PIN, GPIO_PIN_SET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(CUSTOM_VL53L5CX_LPn_PORT, CUSTOM_VL53L5CX_LPn_PIN, GPIO_PIN_SET);
	status = vl53l5cx_is_alive(conf, &isAlive);
	if(!isAlive)
	{
		printf("VL53L5CXV0 not detected at requested address (0x%x)\n", conf->platform.address);
		return 255;
	}
	printf("Sensor initializing, please wait few seconds\n");
	status = vl53l5cx_init(conf);
	status = vl53l5cx_set_ranging_frequency_hz(conf, 2);				// Set 2Hz ranging frequency
	status = vl53l5cx_set_ranging_mode(conf, VL53L5CX_RANGING_MODE_CONTINUOUS);  // Set mode continuous
	return status;
}

void U16_2_U8(const int16_t src, uint8_t* upper, uint8_t* lower) {
	*upper = (uint8_t) (src >> 8);
	*lower = (uint8_t) ((src << 8) >> 8);
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

#ifdef  USE_FULL_ASSERT
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

