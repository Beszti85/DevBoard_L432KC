/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#define USE_SPI_MODULE (0)
#define USE_LCD_MODULE (0)
#define USE_ESP8266    (1)

#include "bme280.h"
#include "mcp23s17.h"
#if (USE_SPI_MODULE == 1)
#include "spi_module.h"
#endif
#include "nrf24l01.h"
//#include "lcd_char.h"
#include "flash.h"
#include "esp8266_at.h"
#include <string.h>
#include "max7219.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticEventGroup_t osStaticEventGroupDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define VCOM_UART_DMA_BUFFER_SIZE 256u

#define ESP_UART_DMA_BUFFER_SIZE 512u
#define ESP_RX_BUFFER_SIZE       2048u

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_rx;

/* Definitions for Task100ms */
osThreadId_t Task100msHandle;
uint32_t Task100msBuffer[ 128 ];
osStaticThreadDef_t Task100msControlBlock;
const osThreadAttr_t Task100ms_attributes = {
  .name = "Task100ms",
  .cb_mem = &Task100msControlBlock,
  .cb_size = sizeof(Task100msControlBlock),
  .stack_mem = &Task100msBuffer[0],
  .stack_size = sizeof(Task100msBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task1sec */
osThreadId_t Task1secHandle;
uint32_t Task1secBuffer[ 128 ];
osStaticThreadDef_t Task1secControlBlock;
const osThreadAttr_t Task1sec_attributes = {
  .name = "Task1sec",
  .cb_mem = &Task1secControlBlock,
  .cb_size = sizeof(Task1secControlBlock),
  .stack_mem = &Task1secBuffer[0],
  .stack_size = sizeof(Task1secBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TaskComm */
osThreadId_t TaskCommHandle;
uint32_t TaskCommBuffer[ 256 ];
osStaticThreadDef_t TaskCommControlBlock;
const osThreadAttr_t TaskComm_attributes = {
  .name = "TaskComm",
  .cb_mem = &TaskCommControlBlock,
  .cb_size = sizeof(TaskCommControlBlock),
  .stack_mem = &TaskCommBuffer[0],
  .stack_size = sizeof(TaskCommBuffer),
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for EventComTask */
osEventFlagsId_t EventComTaskHandle;
osStaticEventGroupDef_t myEvent01ControlBlock;
const osEventFlagsAttr_t EventComTask_attributes = {
  .name = "EventComTask",
  .cb_mem = &myEvent01ControlBlock,
  .cb_size = sizeof(myEvent01ControlBlock),
};
/* USER CODE BEGIN PV */
volatile uint16_t ADC_RawData[4u] = {0u};
float ADC_Voltage[4u];

NRF24L01_Handler_t RFHandler =
{
  .ptrHSpi = &hspi1,
  .portCS  = CS_NRF24L01_GPIO_Port,
  .pinCS   = CS_NRF24L01_Pin,
  .portCE  = CE_NRF24L01_GPIO_Port,
  .pinCE   = CE_NRF24L01_Pin
};

FLASH_Handler_t FlashHandler =
{
  .ptrHSpi = &hspi1,
  .portCS  = CS_FLASH_GPIO_Port,
  .pinCS   = CS_FLASH_Pin
};

MAX7219_Handler_t LedDriverHandler =
{
  .ptrHSpi = &hspi1,
};

char VComDmaBuffer[VCOM_UART_DMA_BUFFER_SIZE];

char EspDmaBuffer[ESP_UART_DMA_BUFFER_SIZE];
char EspRxBuffer[ESP_RX_BUFFER_SIZE];

uint16_t oldPos = 0;
uint16_t newPos = 0;

bool  ESP_ResponseOK = 0u;
bool  ESP_MessageReceived = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
void StartTask100ms(void *argument);
void StartTask1sec(void *argument);
void StartTaskComm(void *argument);

/* USER CODE BEGIN PFP */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  ESP_ResponseOK = false;
  if (huart->Instance == USART1)
  {
    /* Check if it is an OK: dont need to copy, just set the acknowledge flag  */
    if( strncmp(EspDmaBuffer, "\r\nOK\r\n", 6) )
    {
      ESP_ResponseOK = true;
    }
    else
    {
      oldPos = newPos;  // Update the last position before copying new data

      /* If the data in large and it is about to exceed the buffer size, we have to route it to the start of the buffer
       * This is to maintain the circular buffer
       * The old data in the main buffer will be overlapped
       */
      if (oldPos+Size > ESP_RX_BUFFER_SIZE)  // If the current position + new data size is greater than the main buffer
      {
        uint16_t datatocopy = ESP_RX_BUFFER_SIZE-oldPos;  // find out how much space is left in the main buffer
        memcpy ((uint8_t *)EspRxBuffer+oldPos, EspDmaBuffer, datatocopy);  // copy data in that remaining space

        oldPos = 0;  // point to the start of the buffer
        memcpy ((uint8_t *)EspRxBuffer, (uint8_t *)EspDmaBuffer+datatocopy, (Size-datatocopy));  // copy the remaining data
        newPos = (Size-datatocopy);  // update the position
      }

      /* if the current position + new data size is less than the main buffer
       * we will simply copy the data into the buffer and update the position
       */
      else
      {
        memcpy ((uint8_t *)EspRxBuffer+oldPos, EspDmaBuffer, Size);
        newPos = Size+oldPos;
      }
    }

    /* start the DMA again */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *) EspDmaBuffer, ESP_UART_DMA_BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

    if( ESP_ResponseOK == false )
    {
      ESP_MessageReceived = true;
      osEventFlagsSet(EventComTaskHandle, ESP_EVENT_FLAG_MASK);
    }
  }
  if (huart->Instance == USART2)
  {
    /* start the DMA again */
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *) EspDmaBuffer, ESP_UART_DMA_BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

    osEventFlagsSet(EventComTaskHandle, VCP_EVENT_FLAG_MASK);
  }
}
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
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(1u);
  // Start PWM
  //HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  BME280_Detect();
  BME280_StartMeasurement(Oversampling1, Oversampling1, Oversampling1);
  FLASH_Identification(&FlashHandler);
  NRF24L01_Init(&RFHandler);
#if (USE_SPI_MODULE == 1)
  SPIMODULE_Init(&hspi1, CS_MCP23S17_GPIO_Port, CS_MCP23S17_Pin );
#endif
#if (USE_SPI_LCD == 1)
  // Init LCD
  LcdInit_MSP23S17(LCD_DISP_ON_CURSOR_BLINK, &hspi1, CS_MCP23S17_GPIO_Port, CS_MCP23S17_Pin);
  LcdPuts("Hello_MCP23S17", 0, 0);
#endif
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, VComDmaBuffer, VCOM_UART_DMA_BUFFER_SIZE);
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
#if (USE_ESP8266 == 1)
  ESP8266_Init(&huart1, EspRxBuffer);
  HAL_Delay(10u);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, EspDmaBuffer, ESP_UART_DMA_BUFFER_SIZE);
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
#endif
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Task100ms */
  Task100msHandle = osThreadNew(StartTask100ms, NULL, &Task100ms_attributes);

  /* creation of Task1sec */
  Task1secHandle = osThreadNew(StartTask1sec, NULL, &Task1sec_attributes);

  /* creation of TaskComm */
  TaskCommHandle = osThreadNew(StartTaskComm, NULL, &TaskComm_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of EventComTask */
  EventComTaskHandle = osEventFlagsNew(&EventComTask_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 4;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  sConfig.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  hi2c1.Init.Timing = 0x00B07CB4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, CS_NRF24L01_Pin|CS_FLASH_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CE_NRF24L01_Pin|LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : CS_NRF24L01_Pin CS_FLASH_Pin */
  GPIO_InitStruct.Pin = CS_NRF24L01_Pin|CS_FLASH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : CE_NRF24L01_Pin LD3_Pin */
  GPIO_InitStruct.Pin = CE_NRF24L01_Pin|LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartTask100ms */
/**
  * @brief  Function implementing the Task100ms thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTask100ms */
void StartTask100ms(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    // Convert ADC raw data from last running
    for( uint16_t i = 0u; i < 4u; i++ )
    {
      ADC_Voltage[i] = (float)ADC_RawData[i] * 3.3f / 4096.0f;
    }
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&ADC_RawData[0u], 4u);
    osDelay(100);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask1sec */
/**
* @brief Function implementing the Task1sec thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask1sec */
void StartTask1sec(void *argument)
{
  /* USER CODE BEGIN StartTask1sec */
  /* Infinite loop */
  for(;;)
  {
	HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
    BME280_ReadMeasResult();
    osDelay(1000);
  }
  /* USER CODE END StartTask1sec */
}

/* USER CODE BEGIN Header_StartTaskComm */
/**
* @brief Function implementing the TaskComm thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskComm */
void StartTaskComm(void *argument)
{
  /* USER CODE BEGIN StartTaskComm */
  uint32_t eventFlags = 0u;
  /* Infinite loop */
  for(;;)
  {
    eventFlags = osEventFlagsWait(EventComTaskHandle, ESP_EVENT_FLAG_MASK | VCP_EVENT_FLAG_MASK, osFlagsWaitAny, osWaitForever);
    if( eventFlags |= ESP_EVENT_FLAG_MASK )
    {
      // Process the incoming data that is not OK
      ESP8266_AtReportHandler(EspRxBuffer);
      osEventFlagsClear(EventComTaskHandle, ESP_EVENT_FLAG_MASK);
    }
    if( eventFlags |= VCP_EVENT_FLAG_MASK)
    {
      // Process the incoming frame from PC
      osEventFlagsClear(EventComTaskHandle, VCP_EVENT_FLAG_MASK);
    }
    osDelay(1);
  }
  /* USER CODE END StartTaskComm */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
