/**
 * @file n25q.c
 * @brief N25Q serial flash memory driver implementation.
 *
 * Provides SPI-based read, write, erase, status, and device information
 * operations for the N25Q flash family.
 *
 * @date Nov 1, 2026
 * @author Saeed Ansari
 */

#include <main.h>
#include "spi_port.h"

/**
 * @brief Gets the memory geometry of the selected N25Q device.
 *
 * @param[out] info Pointer to the structure that receives the device information.
 * @return OK on success, or ERR if the output pointer is NULL.
 */
STATUS n25q_get_info(N25Q_Info *info)
{
    if (info == NULL)
    {
        return ERR;
    }

    info->capacity_bytes = N25Q_CAPACITY_BYTES;
    info->page_size      = N25Q_PAGE_SIZE;
    info->subsector_size = N25Q_SUBSECTOR_SIZE;
    info->sector_size    = N25Q_SECTOR_SIZE;

    return OK;
}


/**
 * @brief Reads the manufacturer, memory type, and capacity IDs.
 *
 * @param[out] id Pointer to a three-byte buffer receiving the device ID bytes.
 * @return OK if the complete ID is read successfully, otherwise ERR.
 */
STATUS n25q_read_id(uint8_t *id){

  uint8_t command = ID;
  uint8_t dummy = DUMMY;
  uint8_t ignored;
  uint8_t id_tmp[3];
  STATUS stat = ERR;

  if(id == NULL)
  {
	  return stat;
  }

  write_bit(0U, CLK);
  write_bit(0U, CS);

  do
  {
	  if(spi_transfer(&ignored, &command) != 0U)
	  {
		  break;
	  }
	  if(spi_transfer(&id_tmp[0], &dummy) != 0U)  // Manufacturer ID
	  {
		  break;
	  }
	  if(spi_transfer(&id_tmp[1], &dummy) != 0U) // Memory type
	  {
		  break;
	  }
	  if(spi_transfer(&id_tmp[2], &dummy) != 0U) // Memory capacity
	  {
		  break;
	  }

	  id[0] = id_tmp[0];
	  id[1] = id_tmp[1];
	  id[2] = id_tmp[2];

	  stat = OK;
  }
  while(0);

  write_bit(1U, CS);

  return stat;

}


/**
 * @brief Initializes the N25Q interface and reads its device ID.
 *
 * @return OK if the flash interface is initialized successfully, otherwise ERR.
 */
STATUS n25q_init(void)
{
    uint8_t id[3];

    write_bit(1U, CS);
    write_bit(0U, CLK);

    if (n25q_read_id(id) != OK)
    {
        return ERR;
    }

    /* Match ID against supported devices.
       Store the matching device's memory information.
       Return ERR if the device is unsupported. */

    return OK;
}



/**
 * @brief Reads one status register from the N25Q device.
 *
 * @param commandT Register read command, such as READ_STATUS or READ_FLAG_STATUS.
 * @param[out] status Pointer to the variable that receives the register value.
 * @return OK on success, or ERR if the pointer is NULL or the SPI transfer fails.
 */
STATUS n25q_read_register(uint8_t commandT, uint8_t *status)
{

  uint8_t command = commandT;
  uint8_t dummy = DUMMY;
  uint8_t ignored;
  uint8_t status_tmp;
  STATUS stat = ERR;

  if(status == NULL)
  {
	  return stat;
  }

  write_bit(0U, CLK);
  write_bit(0U, CS);

  do
  {
	  if(spi_transfer(&ignored, &command) != 0U)
	  {
		  break;
	  }
	  if(spi_transfer(&status_tmp, &dummy) != 0U)  // status
	  {
		  break;
	  }


	  *status = status_tmp;


	  stat = OK;
  }
  while(0);

  write_bit(1U, CS);

  return stat;

}



/**
 * @brief Clears the N25Q flag status register.
 *
 * @return OK on success, or ERR if the SPI command fails.
 */
STATUS n25q_clear_flag_status(void)
{
    uint8_t command = CLEAR_FLAG_STATUS;
    uint8_t ignored;

    write_bit(0U, CLK);
    write_bit(0U, CS);

    int result = spi_transfer(&ignored, &command);

    write_bit(1U, CS);

    return (result == 0) ? OK : ERR;
}


/**
 * @brief Waits until the N25Q device completes its write-in-progress operation.
 *
 * @param timeout Maximum wait time in milliseconds.
 * @return OK when the device is ready, or ERR on register-read failure or timeout.
 */
STATUS n25q_wait_ready(uint32_t timeout)
{
	uint32_t time = n25q_port_get_time_ms();
	uint8_t status;

	for(;;)
	{
		if(n25q_read_register(READ_STATUS, &status) != OK)
		{
			return ERR;
		}

		if((status & 1U) == 0U)
		{
			return OK;     //WIP = 0:Ready
		}

		if(n25q_port_get_time_ms() - time >= timeout)
		{
			return ERR;
		}
	}
}


/**
 * @brief Sends the write-enable command and waits for the WEL bit to be set.
 *
 * @param timeout Maximum wait time in milliseconds.
 * @return OK when write operations are enabled, otherwise ERR.
 */
STATUS n25q_write_enable(uint32_t timeout)
{
	  uint8_t command = WRITE_ENABLE;
	  uint8_t ignored;
	  uint8_t status;

	  write_bit(0U, CLK);
	  write_bit(0U, CS);

	  STATUS res = spi_transfer(&ignored, &command);

	  write_bit(1U, CS);

	  if(res != OK)
	  {
		  return ERR;
	  }

	  uint32_t time = n25q_port_get_time_ms();

	  for(;;)
	  {
			if(n25q_read_register(READ_STATUS, &status) != OK)
			{
				return ERR;
			}
			if(((status >> 1U) & 1U) != 0U)
			{
				return OK;     // WEL = 1: writing enabled
			}

			if(n25q_port_get_time_ms() - time >= timeout)
			{
				return ERR;
			}
	  }
}



/**
 * @brief Reads one byte from a 24-bit N25Q memory address.
 *
 * @param address Memory address to read; must not exceed 0xFFFFFF.
 * @param[out] data Pointer to the variable that receives the byte.
 * @return OK if the byte is read successfully, otherwise ERR.
 */
STATUS n25q_read(uint32_t address, uint8_t *data)
{

  uint8_t command = READ;
  uint8_t dummy = DUMMY;
  uint8_t ignored;
  uint8_t data_temp;
  uint8_t address_temp[3];
  STATUS stat = ERR;

  if((data == NULL) || (address > 0xFFFFFFU))
  {
	  return stat;
  }


  address_temp[0] = (uint8_t)(address);
  address_temp[1] = (uint8_t)(address >> 8U);
  address_temp[2] = (uint8_t)(address >> 16U);

  write_bit(0U, CLK);
  write_bit(0U, CS);

  do
  {
	  if(spi_transfer(&ignored, &command) != 0U)
	  {
		  break;
	  }
	  if(spi_transfer(&ignored, &address_temp[2]) != 0U)
	  {
		  break;
	  }
	  if(spi_transfer(&ignored, &address_temp[1]) != 0U)
	  {
		  break;
	  }
	  if(spi_transfer(&ignored, &address_temp[0]) != 0U)
	  {
		  break;
	  }
	  if(spi_transfer(&data_temp, &dummy) != 0U)
	  {
		  break;
	  }

	  *data = data_temp;

	  stat = OK;
  }
  while(0);

  write_bit(1U, CS);

  return stat;
}



/**
 * @brief Programs one data range within a single N25Q page.
 *
 * @param address Starting memory address. The range must remain within one page.
 * @param[in] data Pointer to the bytes to program.
 * @param length Number of bytes to program.
 * @param timeout_ms Maximum time in milliseconds for ready and write operations.
 * @return OK if the page is programmed successfully, otherwise ERR.
 */
STATUS n25q_page_program(uint32_t address,
                         const uint8_t *data,
                         uint32_t length,
                         uint32_t timeout_ms)
{

	  uint8_t command = PAGE_PROGRAM;
	  uint8_t ignored;
	  uint8_t address_temp[3];

	  STATUS stat = ERR;

	    if ((data == NULL) ||
	        (length == 0U) ||
	        (address >= N25Q_CAPACITY_BYTES) ||
	        (address > 0xFFFFFFU))
	    {
	        return stat;
	    }


	    if ((length > (N25Q_CAPACITY_BYTES - address)) ||
	        (length > (0x1000000U - address)) ||
	        (length > (N25Q_PAGE_SIZE -
	                   (address % N25Q_PAGE_SIZE))))
	    {
	        return stat;
	    }


	  address_temp[0] = (uint8_t)(address);
	  address_temp[1] = (uint8_t)(address >> 8U);
	  address_temp[2] = (uint8_t)(address >> 16U);


	  STATUS res = n25q_wait_ready(timeout_ms);

	  if(res != OK)
	  {
		  return stat;
	  }

	  res = n25q_write_enable(timeout_ms);

	  if(res != OK)
	  {
		  return stat;
	  }

	  write_bit(0U, CLK);
	  write_bit(0U, CS);


	  if(spi_transfer(&ignored, &command) != 0U)
	  {
		  write_bit(1U, CS);
		  return stat;
	  }
	  if(spi_transfer(&ignored, &address_temp[2]) != 0U)
	  {
		  write_bit(1U, CS);
		  return stat;
	  }
	  if(spi_transfer(&ignored, &address_temp[1]) != 0U)
	  {
		  write_bit(1U, CS);
		  return stat;
	  }
	  if(spi_transfer(&ignored, &address_temp[0]) != 0U)
	  {
		  write_bit(1U, CS);
		  return stat;
	  }

	  for(uint32_t i=0;i<length;i++)
	  {
		  if(spi_transfer(&ignored, data) != 0U)
		  {
			  write_bit(1U, CS);
			  return stat;
		  }
		  data++;
	  }


	  write_bit(1U, CS);

	  res = n25q_wait_ready(timeout_ms);

	  if(res != OK)
	  {
		  return ERR;
	  }

	  stat = OK;

	  return stat;
}



/**
 * @brief Writes data to memory, automatically splitting it across page boundaries.
 *
 * @param address Starting memory address.
 * @param[in] data Pointer to the data to write.
 * @param length Number of bytes to write.
 * @param timeout_ms Maximum time in milliseconds for each page operation.
 * @return OK if all bytes are programmed successfully, otherwise ERR.
 */
STATUS n25q_write(uint32_t address,
                  const uint8_t *data,
                  uint32_t length,
                  uint32_t timeout_ms)
{
    if ((data == NULL) ||
        (length == 0U) ||
        (address >= N25Q_CAPACITY_BYTES) ||
        (address > 0xFFFFFFU))
    {
        return ERR;
    }

    if ((length > (N25Q_CAPACITY_BYTES - address)) ||
        (length > (0x1000000U - address)))
    {
        return ERR;
    }

    while (length > 0U)
    {
        uint32_t available = N25Q_PAGE_SIZE -
                            (address % N25Q_PAGE_SIZE);

        uint32_t chunk = (length < available)
                         ? length : available;

        if (n25q_page_program(address, data,
                             chunk, timeout_ms) != OK)
        {
            return ERR;
        }

        address += chunk;
        data    += chunk;
        length  -= chunk;
    }

    return OK;
}


/**
 * @brief Erases the subsector containing the specified address.
 *
 * @param address Start address of the subsector to erase; it must be subsector-aligned.
 * @param timeout_ms Maximum time in milliseconds for status and erase operations.
 * @return OK if the erase succeeds, otherwise ERR.
 */
STATUS n25q_erase_subsector(uint32_t address,
                            uint32_t timeout_ms)
{

	uint8_t command = SUBSECTOR_ERASE;
	uint8_t ignored;
	uint8_t status;
	uint8_t address_temp[3];

	address_temp[0] = (uint8_t)(address);
	address_temp[1] = (uint8_t)(address >> 8U);
	address_temp[2] = (uint8_t)(address >> 16U);

	if((address >= N25Q_CAPACITY_BYTES) || (address > 0xFFFFFFU))
	{
		return ERR;
	}

	if((address % N25Q_SUBSECTOR_SIZE) != 0U)
	{
		return ERR;
	}

	if(n25q_wait_ready(timeout_ms) != OK)
	{
		return ERR;
	}

	if (n25q_clear_flag_status() != OK)
	{
	    return ERR;
	}

	if(n25q_write_enable(timeout_ms) != OK)
	{
		return ERR;
	}

	write_bit(0U, CLK);
	write_bit(0U, CS);

	if(spi_transfer(&ignored, &command) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}
	if(spi_transfer(&ignored, &address_temp[2]) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}
	if(spi_transfer(&ignored, &address_temp[1]) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}
	if(spi_transfer(&ignored, &address_temp[0]) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}

	write_bit(1U, CS);

	if (n25q_wait_ready(timeout_ms) != OK)
	{
	    return ERR;
	}

	if (n25q_read_register(READ_FLAG_STATUS, &status) != OK)
	{
	    return ERR;
	}

	if ((status & ((1U << 5U) | (1U << 3U) | (1U << 1U))) != 0U)
	{
	    return ERR;
	}

	return OK;
}


/**
 * @brief Erases the sector containing the specified address.
 *
 * @param address Start address of the sector to erase; it must be sector-aligned.
 * @param timeout_ms Maximum time in milliseconds for status and erase operations.
 * @return OK if the erase succeeds, otherwise ERR.
 */
STATUS n25q_erase_sector(uint32_t address,
                         uint32_t timeout_ms)
{

	uint8_t command = SECTOR_ERASE;
	uint8_t ignored;
	uint8_t status;
	uint8_t address_temp[3];

	address_temp[0] = (uint8_t)(address);
	address_temp[1] = (uint8_t)(address >> 8U);
	address_temp[2] = (uint8_t)(address >> 16U);

	if((address >= N25Q_CAPACITY_BYTES) || (address > 0xFFFFFFU))
	{
		return ERR;
	}

	if((address % N25Q_SECTOR_SIZE) != 0U)
	{
		return ERR;
	}

	if(n25q_wait_ready(timeout_ms) != OK)
	{
		return ERR;
	}

	if (n25q_clear_flag_status() != OK)
	{
	    return ERR;
	}

	if(n25q_write_enable(timeout_ms) != OK)
	{
		return ERR;
	}

	write_bit(0U, CLK);
	write_bit(0U, CS);

	if(spi_transfer(&ignored, &command) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}
	if(spi_transfer(&ignored, &address_temp[2]) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}
	if(spi_transfer(&ignored, &address_temp[1]) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}
	if(spi_transfer(&ignored, &address_temp[0]) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}

	write_bit(1U, CS);

	if (n25q_wait_ready(timeout_ms) != OK)
	{
	    return ERR;
	}

	if(n25q_read_register(READ_FLAG_STATUS, &status) != OK)
	{
		return ERR;
	}

	if ((status & ((1U << 5U) | (1U << 3U) | (1U << 1U))) != 0U)
	{
	    return ERR;
	}

	return OK;

}


/**
 * @brief Erases the entire N25Q memory device.
 *
 * @param timeout_ms Maximum time in milliseconds for status and erase operations.
 * @return OK if the bulk erase succeeds, otherwise ERR.
 */
STATUS n25q_erase_chip(uint32_t timeout_ms)
{
	uint8_t command = BULK_ERASE;
	uint8_t ignored;
	uint8_t status;


	if(n25q_wait_ready(timeout_ms) != OK)
	{
		return ERR;
	}

	if (n25q_clear_flag_status() != OK)
	{
	    return ERR;
	}

	if(n25q_write_enable(timeout_ms) != OK)
	{
		return ERR;
	}

	write_bit(0U, CLK);
	write_bit(0U, CS);

	if(spi_transfer(&ignored, &command) != 0U)
	{
	  write_bit(1U, CS);
	  return ERR;
	}

	write_bit(1U, CS);

	if (n25q_wait_ready(timeout_ms) != OK)
	{
	    return ERR;
	}

	if(n25q_read_register(READ_FLAG_STATUS, &status) != OK)
	{
		return ERR;
	}

	if ((status & ((1U << 5U) | (1U << 3U) | (1U << 1U))) != 0U)
	{
	    return ERR;
	}

	return OK;
}

