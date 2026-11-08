/**
 * @file spi_port.c
 * @brief Hardware-specific SPI port layer.
 *
 * This layer adapts the SPI driver to different SoCs. These functions encapsulate
 * the SoC-dependent GPIO, timer, and timing operations, so the SPI driver remains
 * portable and can be reused by replacing the implementations for the target SoC.
 *
 * @date Nov 1, 2026
 * @author Saeed Ansari
 */
#include "spi.h"
#include <stdbool.h>

extern TIM_HandleTypeDef htim1;       //This line is specific to ST microcontrollers, so you may remove it
									  //for your SoC.

/**
 * @brief Writes a logic level to an SPI port pin.
 *
 * @param value Logic level to write; a nonzero value writes high and zero writes low.
 * @param pins SPI pin to control. Supported pins are MOSI, CS, and CLK.
 */
void write_bit(uint8_t value, SPI_PINS pins)
{

	bool pin = (bool)(value);

	switch(pins)
	{
		case MOSI:
			if(pin)
			{HAL_GPIO_WritePin(QSPI_MOSI_GPIO_Port, QSPI_MOSI_Pin, GPIO_PIN_SET);}
			else
			{HAL_GPIO_WritePin(QSPI_MOSI_GPIO_Port, QSPI_MOSI_Pin, GPIO_PIN_RESET);}
			break;
		case CS:
			if(pin)
			{HAL_GPIO_WritePin(QSPI_CS_GPIO_Port, QSPI_CS_Pin, GPIO_PIN_SET);}
			else
			{HAL_GPIO_WritePin(QSPI_CS_GPIO_Port, QSPI_CS_Pin, GPIO_PIN_RESET);}
			break;
		case CLK:
			if(pin)
			{HAL_GPIO_WritePin(QSPI_CLK_GPIO_Port, QSPI_CLK_Pin, GPIO_PIN_SET);}
			else
			{HAL_GPIO_WritePin(QSPI_CLK_GPIO_Port, QSPI_CLK_Pin, GPIO_PIN_RESET);}
			break;
		default:
	}
}

/**
 * @brief Reads the logic level from an SPI port pin.
 *
 * @param pins SPI pin to read. Only the MISO pin is supported.
 *
 * @return 1 when the pin is high, or 0 when it is low or unsupported.
 */
uint8_t read_bit(SPI_PINS pins)
{
	if(pins == MISO)
	{
        return (HAL_GPIO_ReadPin(QSPI_MISO_GPIO_Port, QSPI_MISO_Pin)
                == GPIO_PIN_SET) ? 1U : 0U;
	}

	return 0U;
}


/**
 * @brief Gets the system time in milliseconds.
 *
 * @return Current system uptime in milliseconds.
 */
uint32_t n25q_port_get_time_ms(void)
{
    return HAL_GetTick();
}


/**
 * @brief Blocks execution for an approximate duration in microseconds.
 *
 * The delay uses the counter of TIM1 and returns immediately when the requested
 * duration is zero.
 *
 * @param us Requested delay duration in microseconds.
 */
void microsecond_delay(uint16_t us)
{
	uint16_t count = __HAL_TIM_GET_COUNTER(&htim1);

	if(us == 0U)
	{
		return;
	}

	while((uint16_t)(__HAL_TIM_GET_COUNTER(&htim1) - count) < us)
	{
		//Wait here
	}
}
