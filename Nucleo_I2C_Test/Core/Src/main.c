/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// MMA8452Q accelerometer slave address (shifted already)
static const uint8_t I2C_ADDR = (0x1D) << 1;
static const uint8_t I2C_ADDR_WRITE = I2C_ADDR;
static const uint8_t I2C_ADDR_READ = I2C_ADDR | 1;
// MMA8452Q accelerometer configuration register addresses
static const uint8_t ADDR_CTRL_REG1 = 0x2A;
static const uint8_t ADDR_XYZ_DATA_CFG = 0x0E;
static const uint8_t ADDR_WHO_AM_I = 0x0D;
// MMA8452Q accelerometer data register addresses
static const uint8_t ADDR_STATUS = 0x00;
static const uint8_t ADDR_OUT_X_MSB = 0x01;
//static const uint8_t ADDR_OUT_X_LSB = 0x02;
//static const uint8_t ADDR_OUT_Y_MSB = 0x03;
//static const uint8_t ADDR_OUT_Y_LSB = 0x04;
//static const uint8_t ADDR_OUT_Z_MSB = 0x05;
//static const uint8_t ADDR_OUT_Z_LSB = 0x06;

// I2C Timeout
static const uint32_t I2C_TIMEOUT = 50;

// raw data -> g's conversion array
static const double conversion_array[10] = {5000, 2500, 1250, 625, 313, 156, 78, 39, 20, 10};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void I2C_error_handler(HAL_StatusTypeDef ret, const char* msg);

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
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  // Test I2C connection
  uint8_t who_am_i_val;
  HAL_StatusTypeDef ret;
  ret = HAL_I2C_Mem_Read(&hi2c1, I2C_ADDR_READ, (uint8_t)ADDR_WHO_AM_I, 1, &who_am_i_val, 1, I2C_TIMEOUT);
  I2C_error_handler(ret, "Failed I2C Connection");
  printf("WHO_AM_I: 0x%x\n", who_am_i_val);
  printf("---------------------\n");
  fflush(stdout);

  // Enable standby mode
  uint8_t ctrl_reg1_val;
  ret = HAL_I2C_Mem_Read(&hi2c1, I2C_ADDR_READ, (uint8_t)ADDR_CTRL_REG1, 1, &ctrl_reg1_val, 1, I2C_TIMEOUT);
  I2C_error_handler(ret, "Error reading from CTRL_REG1");
  ctrl_reg1_val &= ~0x01;
  HAL_I2C_Mem_Write(&hi2c1, I2C_ADDR_WRITE, (uint8_t)ADDR_CTRL_REG1, 1, &ctrl_reg1_val, 1, I2C_TIMEOUT);

  // Set 2g dynamic range
  uint8_t xyz_data_cfg_val;
  ret = HAL_I2C_Mem_Read(&hi2c1, I2C_ADDR_READ, (uint8_t)ADDR_XYZ_DATA_CFG, 1, &xyz_data_cfg_val, 1, I2C_TIMEOUT);
  I2C_error_handler(ret, "Error reading from XYZ_DATA_CFG");
  xyz_data_cfg_val &= ~0x03;
  HAL_I2C_Mem_Write(&hi2c1, I2C_ADDR_WRITE, (uint8_t)ADDR_XYZ_DATA_CFG, 1, &xyz_data_cfg_val, 1, I2C_TIMEOUT);

  // Enable active mode
  ret = HAL_I2C_Mem_Read(&hi2c1, I2C_ADDR_READ, (uint8_t)ADDR_CTRL_REG1, 1, &ctrl_reg1_val, 1, I2C_TIMEOUT);
  I2C_error_handler(ret, "Error reading from CTRL_REG1");
  ctrl_reg1_val |= 0x01;
  HAL_I2C_Mem_Write(&hi2c1, I2C_ADDR_WRITE, (uint8_t)ADDR_CTRL_REG1, 1, &ctrl_reg1_val, 1, I2C_TIMEOUT);
  HAL_Delay(5000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  /* USER CODE END WHILE */

	  /* USER CODE BEGIN 3 */

	  // poll the status register
	  uint8_t status_reg_val;
	  ret = HAL_I2C_Mem_Read(&hi2c1, I2C_ADDR_READ, (uint8_t)ADDR_STATUS, 1, &status_reg_val, 1, I2C_TIMEOUT);
	  I2C_error_handler(ret, "Error reading from status register\n");
	  // check if 4th LSB ZYXDR is set (counting from 1)
	  if (status_reg_val & (1 << 3)) {
		  // new data available
		  uint8_t xyz_raw_data[6];
		  int16_t xyz_stitched_data[3];
		  ret = HAL_I2C_Mem_Read(&hi2c1, I2C_ADDR_READ, (uint8_t)ADDR_OUT_X_MSB, 1, xyz_raw_data, 6, I2C_TIMEOUT);
		  I2C_error_handler(ret, "Error reading from XYZ acceleration registers");
		  xyz_stitched_data[0] = ((int16_t)xyz_raw_data[0] << 8) | xyz_raw_data[1];
		  xyz_stitched_data[1] = ((int16_t)xyz_raw_data[2] << 8) | xyz_raw_data[3];
		  xyz_stitched_data[2] = ((int16_t)xyz_raw_data[4] << 8) | xyz_raw_data[5];

		  // Convert raw data to g's for each XYZ
		  double xyz_final_g[3];
//		  for (int j = 0; j < 3; j++) {
//			  // get sign and integer first
//			  double sign = 1;
//			  double int_val = 0;
//			  if (xyz_stitched_data[j] & (1 << 15)) {
//				  // if MSB is set
//				  sign = -1;
//			  }
//			  if (xyz_stitched_data[j] & (1 << 14)) {
//				  // if 2nd MSB is set
//				  int_val = 1;
//			  }
//			  // shift the sign and int_val bits away
//			  xyz_stitched_data[j] = xyz_stitched_data[j] << 2;
//
//			  // get fractional part
//			  double fraction = 0;
//			  for (int i = 0; i < 12; i++) {
//				  if (xyz_stitched_data[j] & (1 << i)) {
//					  // if ith bit is set
//					  fraction += conversion_array[i];
//				  }
//			  }
//			  fraction /= 10000;
//			  xyz_final_g[j] = sign * (int_val + fraction);
//		  }
		  xyz_final_g[0] = xyz_stitched_data[0] / 1024;
		  xyz_final_g[1] = xyz_stitched_data[1] / 1024;
		  xyz_final_g[2] = xyz_stitched_data[2] / 1024;

		  printf("X: %lf\n", xyz_final_g[0]);
		  printf("Y: %lf\n", xyz_final_g[1]);
		  printf("Z: %lf\n", xyz_final_g[2]);
		  printf("---------------------\n");
		  HAL_Delay(500);

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

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
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void I2C_error_handler(HAL_StatusTypeDef ret, const char* msg) {
	if (ret != HAL_OK) {
	  printf("%s\n", msg);
	  printf("I2C Error: 0x%lx\n", HAL_I2C_GetError(&hi2c1));
	  printf("---------------------------\n");
	  fflush(stdout);
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
