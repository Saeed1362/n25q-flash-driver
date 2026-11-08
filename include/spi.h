/**
 * @file spi.h
 * @brief Public interface for the software bit-banged SPI implementation.
 *
 * @date Nov 1, 2026
 * @author Saeed Ansari
 */

#ifndef INC_SPI_H_
#define INC_SPI_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SPI signal pin identifiers used by the port layer. */
typedef enum
{
    MISO, /**< Master-in/slave-out data pin. */
    MOSI, /**< Master-out/slave-in data pin. */
    CLK,  /**< Serial clock pin. */
    CS    /**< Chip-select pin. */
} SPI_PINS;

/**
 * @brief Transfers one byte over the software SPI interface.
 * @param[out] rx Pointer to the received byte.
 * @param[in] tx Pointer to the byte to transmit.
 * @return 0 on success, or 1 if a pointer is NULL or the transfer fails.
 */
uint8_t spi_transfer(uint8_t *rx, const uint8_t *tx);

#ifdef __cplusplus
}
#endif

#endif /* INC_SPI_H_ */
