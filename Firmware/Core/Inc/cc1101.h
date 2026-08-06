#ifndef CC1101_H
#define CC1101_H

#include "stm32f4xx_hal.h"

#define CC1101_PARTNUM  0x30U // Part number register
#define CC1101_VERSION  0x31U // Version number register
#define CC1101_PKTLEN   0x06U // Packet length register
#define CC1101_SRES     0x30U // Reset command strobe

HAL_StatusTypeDef CC1101_Reset(SPI_HandleTypeDef *hspi);

HAL_StatusTypeDef CC1101_WriteRegister(
    SPI_HandleTypeDef *hspi,
    uint8_t address,
    uint8_t value
);

HAL_StatusTypeDef CC1101_ReadRegister(
    SPI_HandleTypeDef *hspi,
    uint8_t address,
    uint8_t *value
);

HAL_StatusTypeDef CC1101_ReadStatusRegister(
    SPI_HandleTypeDef *hspi,
    uint8_t address,
    uint8_t *value
);

#endif