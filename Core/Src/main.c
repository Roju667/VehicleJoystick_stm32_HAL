/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "HC-05_V3.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "movement_control.h"
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
HC05_t HC05_1;							// HC05 bluetooth module structure
uint8_t MsgBuffer[64];				// Buffer for message for bluetooth transfer
uint16_t ADCValue[10][2];				// Buffer for samples from joystick ADC
uint16_t ADCValueAvgX, ADCValueAvgY;	// Average value from 10 samples
uint16_t tempADCValueX, tempADCValueY;	// 10 samples sum
uint16_t ServoPositionX, MotorSpeedY;// Values for servo position (X) and motor speed (Y) that will be send to vehicle
uint8_t volatile ReadyToSend;// flag for message send, if 1 then send new command
uint8_t volatile LightsOn;	// flag set/reset by buttton to turn on/off lights
uint8_t volatile MotorOn;				// flag set by button to start motor
uint32_t LastMsgTimer = 0;				// save tick on last message recieved
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */
uint16_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min,
		uint16_t out_max);
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

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
	HC05_Init(&HC05_1, &huart1);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*) ADCValue, 2 * 10);
	ReadyToSend = 1;
	LightsOn = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		// check if there is a acknowledge message from vehicle
		if (HC05_CheckPendingMessages(&HC05_1, MsgBuffer) == HC05_MESSAGEPENDING)
		{
			// clear flag that message is recieved
			HC05_ClearMsgPendingFlag(&HC05_1);

			// check if message is acknowledge OKAY from vehicle
			if (strcmp((char*) MsgBuffer, "OKAY\n") == 0)
			{
				LastMsgTimer = HAL_GetTick();
				// set flag that will trigger another bluetooth transfer
				ReadyToSend = 1;
				// light up LED to see that communication is working
				HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin,
						GPIO_PIN_SET);
			}
		}

		// reset temporary buffer
		tempADCValueX = 0;
		tempADCValueY = 0;

		// sum 10 adc samples and put the value into temporary register
		for (uint8_t i = 0; i < 10; i++)
		{
			tempADCValueY += ADCValue[i][0];
			tempADCValueX += ADCValue[i][1];
		}

		// if there was acknowledge or it is first transfer then trigger transfer routine
		if (ReadyToSend == 1)
		{
			// reset flag
			ReadyToSend = 0;
			// calculate average value x and y from ADC
			ADCValueAvgX = tempADCValueX / 10;
			ADCValueAvgY = tempADCValueY / 10;

			// map ADC X value 0-4095 to servo PWM value 2333-3666 (60-120) degrees
			ServoPositionX = map(ADCValueAvgX, 0, 4095, SERVO_LOW_LIMIT,
			SERVO_HIGH_LIMIT);

			// map ADC Y value 0-4095 to servo PWM value 0-40000
			MotorSpeedY = map(ADCValueAvgY, 0, 4095, MOTOR_LOW_LIMIT,
			MOTOR_HIGH_LIMIT);

			// variable to save lenght of message
			uint8_t len = 0;
			// prepare command ACK;<ServoPosition>;<MotorSpeed>;<FrontLEDsON>;<MotorONOff>;
			len = sprintf((char*) MsgBuffer, "ACK;%d;%d;%d;%d;\n",
					ServoPositionX, MotorSpeedY, LightsOn, MotorOn);
			// send command to vehicle
			HAL_UART_Transmit(&huart1, MsgBuffer, len, 1000);
			// wait 80 ms before next transfer
			HAL_Delay(80);
		}

		// if there was no communication for 3s
		if ((HAL_GetTick() - LastMsgTimer) > 3000)
		{
			//reset LED
			HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, GPIO_PIN_RESET);

			//flush ring buffer
			RB_Flush(&(HC05_1.RingBuffer));
			//send new message command
			ReadyToSend = 1;
			//feed timer
			LastMsgTimer = HAL_GetTick();
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* USART1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* EXTI2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
  /* EXTI1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  /* EXTI0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

/* USER CODE BEGIN 4 */
uint16_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min,
		uint16_t out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#if (HC05_UART_RX_DMA == 1)
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	// Callback from BT module
	HC05_RxCpltCallbackDMA(&HC05_1, huart, Size);

}
#endif

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	// if button is clicked then set flag (restart transfer)
	if (GPIO_Pin == GPIO_PIN_0)
	{
		ReadyToSend = 1;
	}

	// if lights button is pressed
	if (GPIO_Pin == GPIO_PIN_2)
	{
		// toggle lights
		LightsOn ^= 0x01;

	}

	// if motor button is pressed
	if (GPIO_Pin == GPIO_PIN_1)
	{
		// when not pressed (its pulled up) turn off motor
		if (HAL_GPIO_ReadPin(BUTTON_ENGINE_GPIO_Port, BUTTON_ENGINE_Pin)
				== GPIO_PIN_SET)
		{
			MotorOn = 0;
		}
		// if pressed (pulled to gnd) turn on motor
		else
		{
			MotorOn = 1;
		}
	}
}
#if (HC05_UART_RX_IT == 1)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	// Callback from BT module
	HC05_RxCpltCallbackIT(&HC05_1, huart);
}
#endif
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
