/**
 * @file spi_port.h
 * @brief Hardware-port interface for the bit-banged SPI implementation.
 *
 * This portability layer defines the GPIO and timing operations used by the
 * SPI driver. Implement these functions for the target SoC when porting the driver.
 *
 * @date Nov 1, 2026
 * @author Saeed Ansari
 */

#ifndef INC_SPI_PORT_H_
#define INC_SPI_PORT_H_

#include <stdint.h>
#include "spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Writes a logic level to an SPI pin.
 * @param value Logic level to write; zero is low and nonzero is high.
 * @param pins SPI pin to control.
 */
void write_bit(uint8_t value, SPI_PINS pins);

/**
 * @brief Reads the logic level of an SPI pin.
 * @param pins SPI pin to read.
 * @return 1 when the pin is high, otherwise 0.
 */
uint8_t read_bit(SPI_PINS pins);

/**
 * @brief Gets the system time in milliseconds.
 * @return Current system uptime in milliseconds.
 */
uint32_t n25q_port_get_time_ms(void);

/**
 * @brief Blocks for an approximate microsecond duration.
 * @param us Delay duration in microseconds.
 */
void microsecond_delay(uint16_t us);

#ifdef __cplusplus
}
#endif

#endif /* INC_SPI_PORT_H_ */
