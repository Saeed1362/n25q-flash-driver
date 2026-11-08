/**
 * @file spi.c
 * @brief Software bit-banged SPI byte transfer implementation.
 *
 * Implements an SPI byte transfer using the platform-provided port functions
 * for GPIO control and microsecond timing.
 *
 * @date Nov 1, 2026
 * @author Saeed Ansari
 */

#include "spi_port.h"

/**
 * @brief Transfers one byte over the SPI pins using bit-banging.
 *
 * The function sends one byte from the caller-provided transmit buffer and
 * captures one byte from the MISO pin, least-significant bit timing is handled
 * most-significant bit first because the loop starts at bit 7.
 *
 * @param[out] rx Pointer to the variable that receives the transmitted byte.
 * @param[in] tx Pointer to the byte to transmit.
 *
 * @return 0 if the transfer succeeds, or 1 if either pointer is NULL.
 */
uint8_t spi_transfer(uint8_t *rx, const uint8_t *tx)
{
    uint8_t rx_temp = 0;

    if(tx == NULL || rx == NULL)
    {
        return 1;
    }

    for(int8_t bit=7;bit>=0;bit--)
    {

        write_bit((*tx>>bit)&1U, MOSI);
        microsecond_delay(1U);

        write_bit(1, CLK);
        microsecond_delay(1U);

        rx_temp = (
            (rx_temp << 1U) | (read_bit(MISO) & 1U)
        );

        write_bit(0, CLK);
        microsecond_delay(1U);
    }

    *rx = rx_temp;

    return 0;
}
