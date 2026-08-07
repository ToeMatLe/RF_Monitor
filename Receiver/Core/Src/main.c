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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cc1101.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* CC1101 modem registers used by the RSSI receiver test. */
#define CC1101_FSCTRL1       0x0BU
#define CC1101_MDMCFG4       0x10U
#define CC1101_MDMCFG3       0x11U
#define CC1101_MDMCFG2       0x12U
#define CC1101_DEVIATN       0x15U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

static const uint8_t uart_banner[] =
    "\r\nUART2 ready at 115200 8-N-1\r\n";

HAL_UART_Transmit(
    &huart2,
    (uint8_t *)uart_banner, // Address of the message
    sizeof(uart_banner) - 1U, // Minus 1 to exclude the null terminator
    HAL_MAX_DELAY
);

// Report Buffer
char message[320];

uint8_t part_number = 0;
uint8_t version = 0;
uint8_t packet_length_before = 0;
uint8_t packet_length_after = 0;
uint8_t packet_length_restored = 0;

HAL_StatusTypeDef reset_result;
HAL_StatusTypeDef part_result;
HAL_StatusTypeDef version_result;
HAL_StatusTypeDef write_result;

reset_result = CC1101_Reset(&hspi1);

part_result = CC1101_ReadStatusRegister(
    &hspi1,
    CC1101_PARTNUM,
    &part_number
);

version_result = CC1101_ReadStatusRegister(
    &hspi1,
    CC1101_VERSION,
    &version
);

/* Verify ordinary register reading and writing. */

CC1101_ReadRegister(
    &hspi1,
    CC1101_PKTLEN,
    &packet_length_before
);

write_result = CC1101_WriteRegister(
    &hspi1,
    CC1101_PKTLEN,
    0xA5
);

CC1101_ReadRegister(
    &hspi1,
    CC1101_PKTLEN,
    &packet_length_after
);

/* Restore the reset value. */

CC1101_WriteRegister(
    &hspi1,
    CC1101_PKTLEN,
    packet_length_before
);

CC1101_ReadRegister(
    &hspi1,
    CC1101_PKTLEN,
    &packet_length_restored
);

int length = snprintf(
    message,
    sizeof(message),
    "\r\nCC1101 connection test\r\n"
    "Reset status: %d\r\n"
    "PARTNUM status: %d, value: 0x%02X\r\n"
    "VERSION status: %d, value: 0x%02X\r\n"
    "PKTLEN before: 0x%02X\r\n"
    "PKTLEN written: 0x%02X\r\n"
    "PKTLEN restored: 0x%02X\r\n"
    "Write status: %d\r\n",
    reset_result,
    part_result,
    part_number,
    version_result,
    version,
    packet_length_before,
    packet_length_after,
    packet_length_restored,
    write_result
);

if (length > 0)
{
    uint16_t transmit_length =
        (length < (int)sizeof(message))
            ? (uint16_t)length
            : (uint16_t)(sizeof(message) - 1U);

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)message,
        transmit_length,
        HAL_MAX_DELAY
    );
}
// Configure the CC1101 carrier frequency for approximately 433.92 MHz. These values assume a 26 MHz CC1101 crystal.
// Frequency = FREQ × 26 MHz / 2^16
// FREQ word = 0x10B071
HAL_StatusTypeDef receiver_result =
    CC1101_WriteRegister(
        &hspi1,
        CC1101_FREQ2,
        0x10
    );

if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_WriteRegister(
        &hspi1,
        CC1101_FREQ1,
        0xB0
    );
}

if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_WriteRegister(
        &hspi1,
        CC1101_FREQ0,
        0x71
    );
}

/* Match the ESP32 test transmitter's 4.8-kBaud 2-FSK settings. */
if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_WriteRegister(
        &hspi1,
        CC1101_FSCTRL1,
        0x06
    );
}

if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_WriteRegister(
        &hspi1,
        CC1101_MDMCFG4,
        0xA7
    );
}

if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_WriteRegister(
        &hspi1,
        CC1101_MDMCFG3,
        0x83
    );
}

if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_WriteRegister(
        &hspi1,
        CC1101_MDMCFG2,
        0x03
    );
}

if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_WriteRegister(
        &hspi1,
        CC1101_DEVIATN,
        0x15
    );
}

// Stage 2 only measures RSSI. Use asynchronous serial mode so the packet handler and RX FIFO cannot fill with noise or unwanted packets
uint8_t packet_control = 0;

if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_ReadRegister(
        &hspi1,
        CC1101_PKTCTRL0,
        &packet_control
    );
}

if (receiver_result == HAL_OK)
{
    packet_control = (packet_control & (uint8_t) ~CC1101_PKT_FORMAT_MASK) | CC1101_PKT_FORMAT_ASYNC;

    receiver_result = CC1101_WriteRegister(
        &hspi1,
        CC1101_PKTCTRL0,
        packet_control
    );
}

// Enter receive mode
if (receiver_result == HAL_OK)
{
    receiver_result = CC1101_Strobe(
        &hspi1,
        CC1101_SRX
    );
}

//Allow calibration and receiver settling
HAL_Delay(5);

int receiver_length = snprintf(
    message,
    sizeof(message),
    "433.92 MHz receiver configuration status: %d\r\n",
    receiver_result
);

if (receiver_length > 0)
{
    uint16_t receiver_transmit_length =
        receiver_length < (int)sizeof(message)
            ? (uint16_t)receiver_length
            : (uint16_t)(sizeof(message) - 1U);

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)message,
        receiver_transmit_length,
        HAL_MAX_DELAY
    );
}

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (receiver_result == HAL_OK)
    {
        int16_t minimum_rssi_dbm = 0;
        int16_t peak_rssi_dbm = -200;
        uint8_t peak_rssi_raw = 0;
        uint8_t marcstate = 0;
        HAL_StatusTypeDef sample_result = HAL_OK;

        /*
         * Sample continuously for approximately 250 ms and report the peak.
         * This prevents a short RF packet from being missed between UART
         * print intervals.
         */
        for (uint16_t sample = 0; sample < 125; sample++)
        {
            uint8_t rssi_raw = 0;

            sample_result = CC1101_ReadStatusRegister(
                &hspi1,
                CC1101_RSSI,
                &rssi_raw
            );

            if (sample_result != HAL_OK)
            {
                break;
            }

            int16_t rssi_dbm =
                ((int16_t)(int8_t)rssi_raw / 2) - 74;

            if (sample == 0 || rssi_dbm < minimum_rssi_dbm)
            {
                minimum_rssi_dbm = rssi_dbm;
            }

            if (rssi_dbm > peak_rssi_dbm)
            {
                peak_rssi_dbm = rssi_dbm;
                peak_rssi_raw = rssi_raw;
            }

            HAL_Delay(2);
        }

        HAL_StatusTypeDef state_result =
            CC1101_ReadStatusRegister(
                &hspi1,
                CC1101_MARCSTATE,
                &marcstate
            );

        /* Only bits 4:0 contain the radio state. */
        marcstate &= 0x1FU;

        if (state_result == HAL_OK &&
            (marcstate == CC1101_STATE_IDLE ||
             marcstate == CC1101_STATE_RXFIFO_OVERFLOW))
        {
            HAL_StatusTypeDef recovery_result =
                CC1101_Strobe(
                    &hspi1,
                    CC1101_SFRX
                );

            if (recovery_result == HAL_OK)
            {
                recovery_result = CC1101_Strobe(
                    &hspi1,
                    CC1101_SRX
                );
            }

            HAL_Delay(5);
        }

        if (sample_result == HAL_OK &&
            state_result == HAL_OK)
        {
            char rssi_message[128];

            int rssi_length = snprintf(
                rssi_message,
                sizeof(rssi_message),
                "RSSI window: min %d dBm, peak %d dBm "
                "(raw 0x%02X), MARCSTATE 0x%02X%s\r\n",
                (int)minimum_rssi_dbm,
                (int)peak_rssi_dbm,
                (unsigned int)peak_rssi_raw,
                (unsigned int)marcstate,
                marcstate == CC1101_STATE_RX ? " (RX)" : " (NOT RX)"
            );

            if (rssi_length > 0)
            {
                uint16_t rssi_transmit_length =
                    rssi_length < (int)sizeof(rssi_message)
                        ? (uint16_t)rssi_length
                        : (uint16_t)(sizeof(rssi_message) - 1U);

                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)rssi_message,
                    rssi_transmit_length,
                    HAL_MAX_DELAY
                );
            }
        }
        else
        {
            static const uint8_t read_error[] =
                "RSSI or MARCSTATE read failed\r\n";

            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)read_error,
                sizeof(read_error) - 1U,
                HAL_MAX_DELAY
            );
        }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  HAL_GPIO_WritePin(CC1101_CSN_GPIO_Port, CC1101_CSN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CC1101_CSN_Pin */
  GPIO_InitStruct.Pin = CC1101_CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CC1101_CSN_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
