/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
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
#include "fatfs.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
extern "C" {
#include <string.h>
#include <stdio.h>
#include "userUSART.h"
#include "userSD.h"
#include "userWS2812.h"
#include "RF24.h"
}

#include "GyverButton_HALmod.h"
#include "Battery.h"
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
char debug_message[] = "_____________unknown error_____________";
WS2812_t WS2812_ch5;
SD_t SD;
volatile uint32_t user_ms_counter;
uint8_t NRF_status = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
Battery bat(2800, 4200);

GButton butt_OK(BUTTON_OK_GPIO_Port, BUTTON_OK_Pin);
GButton butt_RIGHT(BUTTON_RIGHT_GPIO_Port, BUTTON_RIGHT_Pin);
GButton butt_LEFT(BUTTON_LEFT_GPIO_Port, BUTTON_LEFT_Pin);

uint8_t show_bat_level (void){
	uint8_t bat_level;
	WS2812_clear(&SD);
	WS2812_start(&WS2812_ch5, &SD, &htim4, TIM_CHANNEL_2, to_show_massage);
	do {
		bat_level = bat.level();
		for (int i = bat_level/11; i >= 0; i--) {
			if		(i<3)	WS2812_setColor(&SD, i, RED);
			else if (i<7)	WS2812_setColor(&SD, i, YELLOW);
			else			WS2812_setColor(&SD, i, GREEN);
		}
	} while (butt_RIGHT.isHold() || butt_LEFT.isHold());
	HAL_Delay(2000);
	WS2812_clear(&SD);
	printf("bat_level: %d\r\n", bat_level);
	return HAL_OK;
}

uint8_t show_file_number(void){
	WS2812_clear(&SD);
	if (SD.repeate_mode==repeate_off) {
		WS2812_setColor(&SD, SD.file_num - 1, BLUE);
	}
	else if (SD.repeate_mode==repeate_one) {
		WS2812_setColor(&SD, SD.file_num - 1, GREEN);
	}
	else if (SD.repeate_mode==repeate_all) {
		WS2812_setColor(&SD, SD.file_num - 1, RED);
	}
	return HAL_OK;
}

uint8_t show_alarm(COLORS color){
	WS2812_fill(&SD, color);
	return WS2812_start(&WS2812_ch5, &SD, &htim4, TIM_CHANNEL_2, to_show_massage);
}

uint8_t play (void){
	HAL_TIM_PWM_Stop_DMA(&htim4, TIM_CHANNEL_2);

  	if(SD_prepare_file(&SD)==FR_OK){

	  	if (WS2812_start(&WS2812_ch5, &SD, &htim4, TIM_CHANNEL_2, to_play_file) == HAL_OK){
	  		SD.play_flag = 1;
			#ifdef DEBUG_ON
	  		printf("WS2812_start: OK \r\n");
			#endif
	  		return HAL_OK;
	  	}
	  	else{
	  		printf("WS2812_start: ERROR  \r\n");
	  		return HAL_ERROR;
	  	}
	}
	else{
		show_alarm(RED);
		printf("file prepare ERROR: %s\r\n",SD.file_name);
		return HAL_ERROR;
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
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
	HAL_Delay(100);
	bat.begin(3400, 2, sigmoidal);
	butt_OK.setTickMode(AUTO);
	butt_LEFT.setTickMode(AUTO);
	butt_RIGHT.setTickMode(AUTO);
	butt_OK.setTimeout(3000);//таймаут (мс) удержания кнопки (для остальных по умолчанию он равен 300 мс)

	// чтение файла с настройками из SD карты
	if (SD_read_settings(&SD)!=FR_OK){
		SD.status = HAL_ERROR;
		show_alarm(RED);
		HAL_Delay(500);
		printf("SD_read_settings: ERROR \r\n");;
	}
	else SD.status = HAL_OK;

	// инициализация радиомодуля
	if (NRF_Init(SD.radio_channel_num, 0xE8E8F0F0E2LL) != HAL_OK){
		NRF_status = HAL_ERROR;
		show_alarm(YELLOW);
		printf("NRF_Init: ERROR \r\n");
	}
	else NRF_status = HAL_OK;

	// если все OK
	if(SD.status == HAL_OK && NRF_status == HAL_OK){
		// индикация заряда батареи и номера файла, затем отключение свечения ленты
		show_bat_level();						// 2000 мс
		show_file_number(); HAL_Delay(2000);	// 2000 мс
		WS2812_clear(&SD);
	}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {

	// если воспроизведение запущено и >= половины кольцевого буфера SD карты вычитана в ленту
	if (SD.play_flag==1 && SD.update_flag==1) {
		// обновление кольцевого буфера
		SD.result.update = (FRESULT) SD_update_bufer(&SD);

		// если файл закончился или иная причина невозможности обновить буфер
		if (SD.result.update != FR_OK) {
			WS2812_clear(&SD);//отключение дисплея

			if (SD.repeate_mode==repeate_off) {
				SD.play_flag=0;
				HAL_Delay(100);
				WS2812_start(&WS2812_ch5, &SD, &htim4, TIM_CHANNEL_2, to_show_massage);
				printf("play is stop\r\n");
			}
			else if (SD.repeate_mode==repeate_one) {
				play();//play old file again
				printf("repeat old file %d\r\n", (int) HAL_GetTick());
			}
			else if (SD.repeate_mode==repeate_all) {
				SD_change_file(&SD, +1, 255);
				play();//play new file again
				printf("repeat new file %d\r\n", (int) HAL_GetTick());
			}
		}

		SD.update_flag=0;
	}

	// если клик по кнопке ОК, SD карта и радиомодуль готовы и проигрывание ещё не начато - посылка синхропосылки и старт воспроизведения файла
	if (butt_OK.isClick() && SD.status == HAL_OK && NRF_status == HAL_OK && SD.play_flag == 0) {
		radio_tx_sync ();//duration 50 ms
		play();
		HAL_GPIO_TogglePin(STATUS_GPIO_Port, STATUS_Pin);
	}

	//// если удержание кнопки ОК - остановить воспроизведение
	//if (butt_OK.isHolded() ) {
	//	WS2812_clear(&SD);
	//	SD.play_flag=0;
	//	HAL_Delay(100);
	//	WS2812_start(&WS2812_ch5, &SD, &htim4, TIM_CHANNEL_2, to_show_massage);
	//	printf("play is stop\r\n");
	//	HAL_GPIO_TogglePin(STATUS_GPIO_Port, STATUS_Pin);
	//}
	//____________________________________________________________________________________________________

	// если удерживается нажаттой кнопка ОК - остановка воспроизведения
	if (butt_OK.isHolded()) {
		SD.play_flag=0;
		WS2812_clear(&SD);
		HAL_Delay(100);
		WS2812_start(&WS2812_ch5, &SD, &htim4, TIM_CHANNEL_2, to_show_massage);
		printf("play is stop\r\n");
		HAL_GPIO_TogglePin(STATUS_GPIO_Port, STATUS_Pin);
	}
	////____________________________________________________________________________________________________


	// если клик по кнопке влево и никакой файл НЕ проигрывается
	if (butt_LEFT.isClick() && !SD.play_flag) {
		// попытка переключить на 1 файл назад
		if (SD_change_file(&SD, -1, 255) == HAL_OK){
			show_file_number();
		}
		// если переключить неудалось - индикация аварии
		else {
			show_alarm(RED);
		}
		HAL_GPIO_TogglePin(STATUS_GPIO_Port, STATUS_Pin);
	}
	//____________________________________________________________________________________________________

	// если клик по кнопке вправо и никакой файл НЕ проигрывается
	if (butt_RIGHT.isClick() && !SD.play_flag) {
		// попытка переключить на 1 файл вперёд
		if (SD_change_file(&SD, +1, 255) == HAL_OK){
			show_file_number();
		}
		// если переключить неудалось - индикация аварии
		else {
			show_alarm(RED);
		}
		HAL_GPIO_TogglePin(STATUS_GPIO_Port, STATUS_Pin);
	}
	//____________________________________________________________________________________________________


	// если удерживается нажатой кнопка влево или вправо - показать уровень заряда батареи
	if (butt_RIGHT.isHolded() | butt_LEFT.isHolded() && SD.play_flag==0) {
		show_bat_level();
	}
	//____________________________________________________________________________________________________

	// если SD карта и радиомодуль готовы и по радиоканалу получена синхропосылка - старт воспроизведения файла
	if(NRF_status == HAL_OK && SD.status == HAL_OK && radio_rx_sync () == 1) {//duration 50 ms
		play();
		HAL_GPIO_TogglePin(STATUS_GPIO_Port, STATUS_Pin);
	}
	//____________________________________________________________________________________________________

//	if (user_ms_counter%200 == 0) {
//		user_ms_counter++;
//		NRF_Init(SD.radio_channel_num, 0xE8E8F0F0E2LL);
//		HAL_GPIO_TogglePin(STATUS_GPIO_Port, STATUS_Pin);
//	}

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
	#ifdef DEBUG_ON
	printf("%s\r\n", debug_message);
	strcpy (debug_message, "_____________unknown error_____________");
	#endif
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
