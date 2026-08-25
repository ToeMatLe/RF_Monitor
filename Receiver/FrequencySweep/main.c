#include "main.h"
#include "cc1101.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CC1101_FSCTRL1       0x0BU
#define CC1101_MDMCFG4       0x10U
#define CC1101_MDMCFG3       0x11U
#define CC1101_MDMCFG2       0x12U
#define CC1101_DEVIATN       0x15U
#define CC1101_SCAL          0x33U

#define CC1101_XOSC_HZ       26000000UL
#define SWEEP_START_HZ       433400000UL
#define SWEEP_END_HZ         434400000UL
#define SWEEP_STEP_HZ        10000UL
#define SWEEP_SAMPLES        50U
#define SAMPLE_DELAY_MS      2U
#define RADIO_SETTLE_MS      5U
#define BETWEEN_SWEEPS_MS    2000U

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void UART_Print(const char *text);
static HAL_StatusTypeDef Configure_CC1101(void);
static HAL_StatusTypeDef Tune_CC1101(uint32_t frequency_hz);
static HAL_StatusTypeDef Measure_RSSI(
    int32_t *average_tenths_dbm,
    int16_t *minimum_half_dbm,
    int16_t *peak_half_dbm
);
static void Print_Half_dBm(char *buffer, size_t size, int16_t half_dbm);
static void Run_Sweep(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_SPI1_Init();

    UART_Print("\r\nCC1101 433.4-434.4 MHz RSSI frequency sweep\r\n");
    UART_Print("Keep the test transmitter continuously ON during the sweep.\r\n");

    HAL_StatusTypeDef result = Configure_CC1101();
    if (result != HAL_OK)
    {
        char error_message[80];
        snprintf(
            error_message,
            sizeof(error_message),
            "CC1101 configuration failed, HAL status %d\r\n",
            (int)result
        );
        UART_Print(error_message);
        Error_Handler();
    }

    while (1)
    {
        Run_Sweep();
        HAL_Delay(BETWEEN_SWEEPS_MS);
    }
}

static void UART_Print(const char *text)
{
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)text,
        (uint16_t)strlen(text),
        HAL_MAX_DELAY
    );
}

static HAL_StatusTypeDef Configure_CC1101(void)
{
    HAL_StatusTypeDef result = CC1101_Reset(&hspi1);
    uint8_t packet_control = 0U;

    if (result == HAL_OK)
        result = CC1101_WriteRegister(&hspi1, CC1101_FSCTRL1, 0x06U);
    if (result == HAL_OK)
        result = CC1101_WriteRegister(&hspi1, CC1101_MDMCFG4, 0xA7U);
    if (result == HAL_OK)
        result = CC1101_WriteRegister(&hspi1, CC1101_MDMCFG3, 0x83U);
    if (result == HAL_OK)
        result = CC1101_WriteRegister(&hspi1, CC1101_MDMCFG2, 0x03U);
    if (result == HAL_OK)
        result = CC1101_WriteRegister(&hspi1, CC1101_DEVIATN, 0x15U);
    if (result == HAL_OK)
        result = CC1101_ReadRegister(
            &hspi1,
            CC1101_PKTCTRL0,
            &packet_control
        );
    if (result == HAL_OK)
    {
        packet_control =
            (packet_control & (uint8_t)~CC1101_PKT_FORMAT_MASK) |
            CC1101_PKT_FORMAT_ASYNC;
        result = CC1101_WriteRegister(
            &hspi1,
            CC1101_PKTCTRL0,
            packet_control
        );
    }

    return result;
}

static HAL_StatusTypeDef Tune_CC1101(uint32_t frequency_hz)
{
    uint32_t frequency_word = (uint32_t)(
        ((((uint64_t)frequency_hz) << 16) + (CC1101_XOSC_HZ / 2U)) /
        CC1101_XOSC_HZ
    );

    HAL_StatusTypeDef result = CC1101_Strobe(&hspi1, CC1101_SIDLE);
    if (result == HAL_OK)
        result = CC1101_WriteRegister(
            &hspi1,
            CC1101_FREQ2,
            (uint8_t)((frequency_word >> 16) & 0xFFU)
        );
    if (result == HAL_OK)
        result = CC1101_WriteRegister(
            &hspi1,
            CC1101_FREQ1,
            (uint8_t)((frequency_word >> 8) & 0xFFU)
        );
    if (result == HAL_OK)
        result = CC1101_WriteRegister(
            &hspi1,
            CC1101_FREQ0,
            (uint8_t)(frequency_word & 0xFFU)
        );
    if (result == HAL_OK)
        result = CC1101_Strobe(&hspi1, CC1101_SCAL);

    HAL_Delay(3U);

    if (result == HAL_OK)
        result = CC1101_Strobe(&hspi1, CC1101_SRX);

    HAL_Delay(RADIO_SETTLE_MS);
    return result;
}

static HAL_StatusTypeDef Measure_RSSI(
    int32_t *average_tenths_dbm,
    int16_t *minimum_half_dbm,
    int16_t *peak_half_dbm)
{
    int32_t sum_half_dbm = 0;
    int16_t minimum = INT16_MAX;
    int16_t peak = INT16_MIN;

    for (uint16_t sample = 0U; sample < SWEEP_SAMPLES; sample++)
    {
        uint8_t raw_rssi = 0U;
        HAL_StatusTypeDef result = CC1101_ReadStatusRegister(
            &hspi1,
            CC1101_RSSI,
            &raw_rssi
        );

        if (result != HAL_OK)
            return result;

        /* Raw RSSI is signed two's-complement with 0.5 dB resolution. */
        int16_t half_dbm = (int16_t)(int8_t)raw_rssi - 148;
        sum_half_dbm += half_dbm;

        if (half_dbm < minimum)
            minimum = half_dbm;
        if (half_dbm > peak)
            peak = half_dbm;

        HAL_Delay(SAMPLE_DELAY_MS);
    }

    *average_tenths_dbm =
        (sum_half_dbm * 5) / (int32_t)SWEEP_SAMPLES;
    *minimum_half_dbm = minimum;
    *peak_half_dbm = peak;
    return HAL_OK;
}

static void Print_Half_dBm(char *buffer, size_t size, int16_t half_dbm)
{
    int32_t tenths = (int32_t)half_dbm * 5;
    snprintf(
        buffer,
        size,
        "%ld.%ld",
        (long)(tenths / 10),
        (long)labs(tenths % 10)
    );
}

static void Run_Sweep(void)
{
    int32_t strongest_average = INT32_MIN;
    uint32_t strongest_frequency = SWEEP_START_HZ;
    char line[160];

    UART_Print("\r\n--- sweep start ---\r\n");
    UART_Print("frequency, average RSSI, minimum RSSI, peak RSSI\r\n");

    for (
        uint32_t frequency_hz = SWEEP_START_HZ;
        frequency_hz <= SWEEP_END_HZ;
        frequency_hz += SWEEP_STEP_HZ)
    {
        HAL_StatusTypeDef result = Tune_CC1101(frequency_hz);
        int32_t average_tenths = 0;
        int16_t minimum_half_dbm = 0;
        int16_t peak_half_dbm = 0;

        if (result == HAL_OK)
        {
            result = Measure_RSSI(
                &average_tenths,
                &minimum_half_dbm,
                &peak_half_dbm
            );
        }

        if (result != HAL_OK)
        {
            snprintf(
                line,
                sizeof(line),
                "%lu.%03lu MHz, read error %d\r\n",
                (unsigned long)(frequency_hz / 1000000UL),
                (unsigned long)((frequency_hz % 1000000UL) / 1000UL),
                (int)result
            );
            UART_Print(line);
            continue;
        }

        if (average_tenths > strongest_average)
        {
            strongest_average = average_tenths;
            strongest_frequency = frequency_hz;
        }

        char minimum_text[16];
        char peak_text[16];
        Print_Half_dBm(minimum_text, sizeof(minimum_text), minimum_half_dbm);
        Print_Half_dBm(peak_text, sizeof(peak_text), peak_half_dbm);

        snprintf(
            line,
            sizeof(line),
            "%lu.%03lu MHz, avg %ld.%ld dBm, min %s dBm, peak %s dBm\r\n",
            (unsigned long)(frequency_hz / 1000000UL),
            (unsigned long)((frequency_hz % 1000000UL) / 1000UL),
            (long)(average_tenths / 10),
            (long)labs(average_tenths % 10),
            minimum_text,
            peak_text
        );
        UART_Print(line);
    }

    snprintf(
        line,
        sizeof(line),
        "BEST: %lu.%03lu MHz with average RSSI %ld.%ld dBm\r\n",
        (unsigned long)(strongest_frequency / 1000000UL),
        (unsigned long)((strongest_frequency % 1000000UL) / 1000UL),
        (long)(strongest_average / 10),
        (long)labs(strongest_average % 10)
    );
    UART_Print(line);
    UART_Print("--- sweep complete ---\r\n");
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clocks = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    oscillator.PLL.PLLM = 16;
    oscillator.PLL.PLLN = 336;
    oscillator.PLL.PLLP = RCC_PLLP_DIV4;
    oscillator.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK)
        Error_Handler();

    clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV2;
    clocks.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

static void MX_SPI1_Init(void)
{
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
        Error_Handler();
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
        Error_Handler();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(
        CC1101_CSN_GPIO_Port,
        CC1101_CSN_Pin,
        GPIO_PIN_SET
    );

    gpio.Pin = B1_Pin;
    gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &gpio);

    gpio.Pin = CC1101_CSN_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CC1101_CSN_GPIO_Port, &gpio);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
