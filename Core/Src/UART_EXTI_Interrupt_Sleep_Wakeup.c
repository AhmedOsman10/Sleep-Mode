/* USER CODE BEGIN Header */
/******************************************************************************
 * @file    main.c
 * @author  Ahmed
 * @brief   STM32F407 Low-Power Sleep Mode and Wake-Up Demonstration
 *
 * @details
 * This project demonstrates how to reduce power consumption on the
 * STM32F407 by placing the CPU into Sleep Mode while keeping selected
 * peripherals active and capable of generating wake-up events.
 *
 * Quick Hardware Setup:
 * ---------------------------------------------------------------------------
 * USART3 Communication:
 *   - PB10 -> USART3_TX -> Connect to USB-TTL RX
 *   - PB11 -> USART3_RX -> Connect to USB-TTL TX
 *   - GND  -> Connect USB-TTL GND
 *   - Baud Rate: 115200
 *
 * External Interrupt Wake-Up:
 *   - PB7 configured as EXTI Falling Edge interrupt
 *   - Connect a normally-open push button between PB7 and GND
 *   - Internal Pull-Up resistor enabled in software
 *
 * STM32CubeMX Reminder:
 *   - Configure PB10 as USART3_TX.
 *   - Configure PB11 as USART3_RX.
 *   - Configure PB7 as GPIO_EXTI7.
 *   - Enable USART3 Global Interrupt in NVIC Settings.
 *   - Enable EXTI Line[9:5] Interrupt in NVIC Settings.
 *   - Re-generate the project so the required interrupt handlers are
 *     created in stm32f4xx_it.c and linked to the HAL callbacks.
 * ---------------------------------------------------------------------------
 *
 * The application periodically enters Sleep Mode using the WFI
 * (Wait For Interrupt) instruction after preparing the system for
 * low-power operation. During Sleep Mode, CPU execution is halted,
 * significantly reducing power consumption while allowing interrupt
 * sources to remain operational.
 *
 * Wake-up events are generated from:
 *   - USART3 receive interrupt (incoming serial character)
 *   - PB7 external interrupt (user push-button)
 *
 * Upon a wake-up event:
 *   - The interrupt service routine executes immediately.
 *   - The wake-up source is reported through USART3.
 *   - A diagnostic LED is toggled for visual confirmation.
 *   - System timing is restored and normal execution resumes.
 *
 * Key Learning Objectives:
 *   - Entering Sleep Mode using HAL power functions
 *   - Preventing unwanted wake-ups caused by SysTick
 *   - Configuring UART interrupts as wake-up sources
 *   - Configuring EXTI interrupts as wake-up sources
 *   - Understanding execution flow before and after WFI
 *   - Basic low-power debugging techniques
 *
 * Important Notes:
 *   - SysTick is suspended before entering Sleep Mode to prevent
 *     automatic wake-ups every 1ms.
 *   - UART reception is interrupt-driven and must be re-armed after
 *     every received byte.
 *   - PB7 is configured with an internal pull-up resistor and triggers
 *     on a falling-edge transition.
 *   - PB7 shares functionality with I2C1_SDA and may be overridden if
 *     I2C1 GPIO configuration is enabled later in the application.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
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
I2S_HandleTypeDef hi2s3;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t Rx_data; // Hardware peripheral DMA/Interrupt destination for a single raw UART byte
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  USART3 RX Complete ISR Callback Hook.
  * Executes directly within the high-priority USART3_IRQHandler sequence.
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART3)
  {
    /* DIAGNOSTIC TRANSMISSION: Announce UART wake-up source immediately inside the ISR */
    const char *msg_uart_wake = "[CRITICAL WAKE] Source: USART3 RX Character Received\r\n";
    HAL_UART_Transmit(&huart3, (uint8_t *)msg_uart_wake, strlen(msg_uart_wake), HAL_MAX_DELAY);

    /* RE-ARM WARNING: The HAL receive interrupt is single-shot. Failure to re-arm
       here completely kills future UART wake capability until a system reset occurs. */
    HAL_UART_Receive_IT(&huart3, &Rx_data, 1);

    /* Diagnostic visual flash toggling the PD6 debug terminal LED */
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_6);
  }
}

/**
  * @brief  EXTI Line Edge Detection Callback.
  * Executes directly within the EXTI9_5_IRQHandler hardware sequence.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* VOLTAGE TRANSITION: Fires exactly when PB7 falls from 3.3V to 0V (GND). */
  if (GPIO_Pin == GPIO_PIN_7)
  {
    /* DIAGNOSTIC TRANSMISSION: Announce Push-Button wake-up source immediately inside the ISR */
    const char *msg_btn_wake = "[CRITICAL WAKE] Source: EXTI Line 7 (PB7 Push-Button Press to GND)\r\n";
    HAL_UART_Transmit(&huart3, (uint8_t *)msg_btn_wake, strlen(msg_btn_wake), HAL_MAX_DELAY);

    /* Diagnostic visual confirmation that the button interrupt successfully hit the vector */
    HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_6);
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
  MX_I2C1_Init(); // WARNING: If PB7 fails, verify this function isn't hijacking the pin for I2C use.
  MX_I2S3_Init();
  MX_SPI1_Init();
  MX_USB_HOST_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  /* Activate initial asynchronous background receiver loop to prepare the first wake-up vector */
  HAL_UART_Receive_IT(&huart3, &Rx_data, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */

    /* Phase 1: Establish countdown window before execution freeze */
    const char *msg_sleep = "\r\n[SYSTEM STATUS] Entering Low Power Sleep Mode in 5 Seconds...\r\n";
    HAL_UART_Transmit(&huart3, (uint8_t *)msg_sleep, strlen(msg_sleep), HAL_MAX_DELAY);

    /* Visually represent active countdown window via LED status high */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(5000);

    /* Phase 2: Core preparation for low-power gating
       TIMING FAULT RISK: Systick must be disabled immediately prior to sleep entry.
       Leaving it active causes an automatic wake condition 1 millisecond later. */
    HAL_SuspendTick();

    /* Deassert LED to explicitly establish the moment the CPU clock stalls */
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);

    /* Phase 3: Execute WFI Instruction (Wait For Interrupt)
       HARDWARE STALL POINT: Core clock execution halts entirely on this block line.
       Sub-peripherals (USART3, EXTI Matrix) remain energized to capture incoming wake vectors. */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* =================================================================
       INTERRUPT TRAP ZONE
       When an event occurs, execution jumps immediately to the ISR callbacks
       at the top of this file, processes the print statements, and then lands here.
       ================================================================= */

    /* Phase 4: Recover foundational runtime timing matrix */
    HAL_ResumeTick();

    /* Phase 5: Success indicator loop to signal system recovery */
    for(int i = 0; i < 20; i++)
    {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_6);
        HAL_Delay(150);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_96K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin|GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : CS_I2C_SPI_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PDM_OUT_Pin */
  GPIO_InitStruct.Pin = PDM_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin PD6 */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /* FIXED PB7: Internal Pull-Up, Falling Edge trigger configuration */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt line 1 init */
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  /* Shared Interrupt Line for EXTI Pins 5 through 9 (Handles PB7) */
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
