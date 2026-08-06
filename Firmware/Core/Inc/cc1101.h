#ifndef CC1101_H
#define CC1101_H

#include "stm32f4xx_hal.h"

#define CC1101_PARTNUM 0x30U // Part number register
#define CC1101_VERSION 0x31U // Version number register
#define CC1101_PKTLEN 0x06U // Packet length register

/* Frequency configuration registers. */
#define CC1101_FREQ2 0x0DU
#define CC1101_FREQ1 0x0EU
#define CC1101_FREQ0 0x0FU

/* Read-only status registers. */
#define CC1101_RSSI 0x34U 
#define CC1101_MARCSTATE 0x35U

/* Command strobes. */
#define CC1101_SRES 0x30U  // Reset command strobe
#define CC1101_SRX 0x34U   // Recieve command strobe
#define CC1101_SIDLE 0x36U // Idle command strobe

/* Expected MARCSTATE value while receiving. */
#define CC1101_STATE_RX 0x0DU

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

HAL_StatusTypeDef CC1101_Strobe(
    SPI_HandleTypeDef *hspi,
    uint8_t command
);
#endif