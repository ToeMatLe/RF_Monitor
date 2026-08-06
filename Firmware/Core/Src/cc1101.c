#include "cc1101.h"
#include "main.h"

/* CC1101 SPI Command Definitions 
    Bit 7: Read register
    Bit 6: Burst register
    Bit 5-0: Address
*/
#define CC1101_READ_SINGLE  0x80U // 1000 
#define CC1101_READ_STATUS  0xC0U // 1100
#define CC1101_SPI_TIMEOUT  100U  // milliseconds
#define CC1101_READY_TIMEOUT 20U  // milliseconds

static void CC1101_Deselect(void)
{
    HAL_GPIO_WritePin(
        CC1101_CSN_GPIO_Port,
        CC1101_CSN_Pin,
        GPIO_PIN_SET // pulls high
    );
}

static HAL_StatusTypeDef CC1101_Select(void)
{
    HAL_GPIO_WritePin(
        CC1101_CSN_GPIO_Port,
        CC1101_CSN_Pin,
        GPIO_PIN_RESET // pulls low
    );

    uint32_t start = HAL_GetTick();

    /*
     * CC1101 drives SO/MISO low when it is ready.
     * PA6 is SPI1_MISO on the NUCLEO-F411RE.
     */
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET)
    {
        if ((HAL_GetTick() - start) > CC1101_READY_TIMEOUT)
        {
            CC1101_Deselect();
            return HAL_TIMEOUT;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef CC1101_Reset(SPI_HandleTypeDef *hspi)
{
    uint8_t command = CC1101_SRES; 
    uint8_t status = 0;

    CC1101_Deselect();
    HAL_Delay(1);

    /* CSN pulse before issuing the reset command. */
    HAL_GPIO_WritePin(
        CC1101_CSN_GPIO_Port,
        CC1101_CSN_Pin,
        GPIO_PIN_RESET
    );
    HAL_Delay(1);
    CC1101_Deselect();
    HAL_Delay(1);

    HAL_StatusTypeDef result = CC1101_Select();
    if (result != HAL_OK) return result;

    // Send the reset command and read the status byte.
    result = HAL_SPI_TransmitReceive(
        hspi,
        &command, // Address of the command byte to send
        &status,  // Address where the received status byte is stored
        1,           // Send and receive 1 byte
        CC1101_SPI_TIMEOUT 
    );

    if (result == HAL_OK)
    {
        uint32_t start = HAL_GetTick();

        /* SRES requires waiting for SO to go low again. */
        while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET)
        {
            if ((HAL_GetTick() - start) > CC1101_READY_TIMEOUT)
            {
                result = HAL_TIMEOUT;
                break;
            }
        }
    }

    CC1101_Deselect();
    HAL_Delay(1);

    return result;
}

HAL_StatusTypeDef CC1101_WriteRegister(
    SPI_HandleTypeDef *hspi,
    uint8_t address,
    uint8_t value)
{
    uint8_t data[2] = {
        address & 0x3FU, // 00xx xxxx
        value 
    };

    HAL_StatusTypeDef result = CC1101_Select();

    if (result == HAL_OK)
    {
        result = HAL_SPI_Transmit(
            hspi,
            data,
            sizeof(data),
            CC1101_SPI_TIMEOUT
        );
    }

    CC1101_Deselect();
    return result;
}

HAL_StatusTypeDef CC1101_ReadRegister(
    SPI_HandleTypeDef *hspi,
    uint8_t address,
    uint8_t *value)
{
    uint8_t transmit[2] = {
        address | CC1101_READ_SINGLE, // 1xxx xxxx
        0x00 // dummy byte
    };

    // BUFFER: 0: CC1101 chip status 1: Requested register value
    uint8_t receive[2] = {0};

    HAL_StatusTypeDef result = CC1101_Select();

    if (result == HAL_OK)
    {
        result = HAL_SPI_TransmitReceive(
            hspi,
            transmit,
            receive,
            sizeof(transmit),
            CC1101_SPI_TIMEOUT
        );
    }

    CC1101_Deselect();

    if (result == HAL_OK)
    {
        *value = receive[1];
    }

    return result;
}

HAL_StatusTypeDef CC1101_ReadStatusRegister(
    SPI_HandleTypeDef *hspi,
    uint8_t address,
    uint8_t *value)
{
    uint8_t transmit[2] = {
        address | CC1101_READ_STATUS, // 11xx xxxx
        0x00
    };
    // BUFFER: 0: CC1101 chip status 1: Requested register value
    uint8_t receive[2] = {0};

    HAL_StatusTypeDef result = CC1101_Select();

    if (result == HAL_OK)
    {
        result = HAL_SPI_TransmitReceive(
            hspi,
            transmit,
            receive,
            sizeof(transmit),
            CC1101_SPI_TIMEOUT
        );
    }

    CC1101_Deselect();

    if (result == HAL_OK)
    {
        *value = receive[1];
    }

    return result;
}

HAL_StatusTypeDef CC1101_Strobe(
    SPI_HandleTypeDef *hspi,
    uint8_t command)
{
    uint8_t status = 0;
    HAL_StatusTypeDef result = CC1101_Select();

    if (result == HAL_OK)
    {
        result = HAL_SPI_TransmitReceive(
            hspi,
            &command,
            &status,
            1,
            CC1101_SPI_TIMEOUT
        );
    }

    CC1101_Deselect();
    return result;
}