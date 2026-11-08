/**
 * @file n25q_test.c
 * @brief Functional test suite for the N25Q serial flash driver.
 *
 * Verifies device identification, page programming, multi-page writes,
 * subsector erase, sector erase, and bulk erase behavior over the selected UART.
 *
 * @date Nov 7, 2026
 * @author Saeed Ansari
 */

#include "n25q.h"
#include <stdio.h>
#include <string.h>

#define N25Q_TEST_TIMEOUT_MS       10000U
#define N25Q_TEST_BULK_TIMEOUT_MS  300000U
#define N25Q_TEST_ENABLE_BULK      0U

extern UART_HandleTypeDef huart6;

/**
 * @brief Transmits a null-terminated string over UART.
 *
 * @param str Pointer to the string to transmit. NULL and empty strings are ignored.
 */
void iprint(const char * str)
{

	if(str == NULL)
	{
		return;
	}

	size_t len = strlen(str);

	if((len == 0) || (len > UINT16_MAX))
	{
		return;
	}

	HAL_UART_Transmit(&huart6,
				(uint8_t*)str,
				(uint16_t)len, 100U);
}

/**
 * @brief Verifies that a memory range contains only erased bytes.
 *
 * Reports the address and value of the first mismatch, or the first read failure,
 * through the UART test output.
 *
 * @param address Starting memory address.
 * @param length Number of bytes to verify.
 * @return OK when every byte is 0xFF, otherwise ERR.
 */
static STATUS test_erased(uint32_t address, uint32_t length)
{
    uint8_t value;
    char message[40];

    for (uint32_t i = 0U; i < length; i++)
    {
        if (n25q_read(address + i, &value) != OK)
        {
        	snprintf(message, sizeof(message),
        		   "ID: %02lX\r\n",
				   (unsigned long)(address + i));

        	iprint(message);

            return ERR;
        }

        if (value != 0xFFU)
        {
        	snprintf(message, sizeof(message),
        		   "ID: %02lX %02X\r\n",
                   (unsigned long)(address + i),
                   (unsigned int)value);

        	iprint(message);

            return ERR;
        }
    }

    return OK;
}

/**
 * @brief Compares a memory range with an expected byte pattern.
 *
 * Reports the first mismatch address, expected byte, and actual byte through
 * the UART test output.
 *
 * @param address Starting memory address.
 * @param[in] expected Pointer to the expected data.
 * @param length Number of bytes to compare.
 * @return OK when all bytes match, otherwise ERR.
 */
static STATUS test_compare(uint32_t address,
                           const uint8_t *expected,
                           uint32_t length)
{
    uint8_t actual;
    char message[40];

    for (uint32_t i = 0U; i < length; i++)
    {
        if (n25q_read(address + i, &actual) != OK)
        {
            return ERR;
        }

        if (actual != expected[i])
        {
        	snprintf(message, sizeof(message),
        		   "ID: %02lX %02X %02X\r\n",
				   (unsigned long)(address + i),
				   (unsigned int)expected[i],
				   (unsigned int)actual);

        	iprint(message);

            return ERR;
        }
    }

    return OK;
}


/**
 * @brief Runs the N25Q identification, programming, and erase test sequence.
 *
 * The test erases flash areas and writes nonuniform data patterns as part of
 * the validation sequence. Results are reported over UART.
 *
 * @return OK when all enabled tests pass, otherwise ERR at the first failure.
 */
STATUS n25q_test(void)
{
    uint8_t id[3];
    uint8_t value = 0xA5U;
    char message[40];

    static uint8_t pattern[(2U * N25Q_PAGE_SIZE) + 62U]; //62 is an arbitrary value. I wanted to make sure the buffer is longer than two full pages so I exercise a partial final write
    iprint("\r\nN25Q test: first sector will be erased.\r\n");

    /* 1. Initialization and identification */
    if (n25q_read_id(id) != OK)
    {
        iprint("[FAIL] QSPI chip was not detected / ID\r\n");
        return ERR;
    }

	snprintf(message, sizeof(message),
		   "ID: %02X %02X %02X\r\n",
		   (unsigned int)id[0],
		   (unsigned int)id[1],
		   (unsigned int)id[2]);

	iprint(message);

    /* Prepare a nonuniform test pattern. */
    for (uint32_t i = 0U; i < sizeof(pattern); i++)
    {
        pattern[i] = (uint8_t)(i ^ (i >> 8U) ^ 0x5AU);
    }

    /* 2. Prepare the test sector */
    if (n25q_erase_sector(0U, N25Q_TEST_TIMEOUT_MS) != OK ||
        test_erased(0U, N25Q_SECTOR_SIZE) != OK)
    {
        iprint("[FAIL] Initial sector erase\r\n");
        return ERR;
    }

    /* 3. Single-byte programming */
    if (n25q_page_program(0U, &value, 1U,
                          N25Q_TEST_TIMEOUT_MS) != OK ||
        test_compare(0U, &value, 1U) != OK)
    {
        iprint("[FAIL] Single-byte program\r\n");
        return ERR;
    }

    iprint("[PASS] Single-byte program\r\n");

    /* 4. Full-page programming in the next page */
    if (n25q_page_program(N25Q_PAGE_SIZE,
                          pattern,
                          N25Q_PAGE_SIZE,
                          N25Q_TEST_TIMEOUT_MS) != OK ||
        test_compare(N25Q_PAGE_SIZE,
                     pattern, N25Q_PAGE_SIZE) != OK)
    {
        iprint("[FAIL] Full-page program\r\n");
        return ERR;
    }

    iprint("[PASS] Full-page program\r\n");

    /*
     * 5. Write across page boundaries.
     * Use an untouched area, starting 7 bytes before a page ends.
     */
    uint32_t start = (3U * N25Q_PAGE_SIZE) - 7U;

    if (n25q_write(start, pattern, sizeof(pattern),
                   N25Q_TEST_TIMEOUT_MS) != OK ||
        test_compare(start, pattern, sizeof(pattern)) != OK)
    {
        iprint("[FAIL] Write across pages\r\n");
        return ERR;
    }

    iprint("[PASS] Write across pages\r\n");

    /* 6. Subsector erase: contains the programmed test data */
    if (n25q_erase_subsector(0U,
                             N25Q_TEST_TIMEOUT_MS) != OK ||
        test_erased(0U, N25Q_SUBSECTOR_SIZE) != OK)
    {
        iprint("[FAIL] Subsector erase\r\n");
        return ERR;
    }

    iprint("[PASS] Subsector erase\r\n");

    /* 7. Program both ends of the sector before erasing it */
    value = 0x00U;

    if (n25q_page_program(0U, &value, 1U,
                          N25Q_TEST_TIMEOUT_MS) != OK ||
        n25q_page_program(N25Q_SECTOR_SIZE - 1U,
                          &value, 1U,
                          N25Q_TEST_TIMEOUT_MS) != OK ||
        test_compare(0U, &value, 1U) != OK ||
        test_compare(N25Q_SECTOR_SIZE - 1U,
                     &value, 1U) != OK ||
        n25q_erase_sector(0U,
                          N25Q_TEST_TIMEOUT_MS) != OK ||
        test_erased(0U, N25Q_SECTOR_SIZE) != OK)
    {
        iprint("[FAIL] Sector erase\r\n");
        return ERR;
    }

    iprint("[PASS] Sector erase\r\n");


    const uint32_t locations[3] =
    {
        0U,
        N25Q_CAPACITY_BYTES / 2U,
        N25Q_CAPACITY_BYTES - 1U
    };

    uint8_t test_value = 0x00U;

    for (uint32_t i = 0U; i < 3U; i++)
    {
        uint32_t subsector_start =
            locations[i] -
            (locations[i] % N25Q_SUBSECTOR_SIZE);

        if (n25q_erase_subsector(subsector_start,
                                 N25Q_TEST_TIMEOUT_MS) != OK ||
            n25q_page_program(locations[i], &test_value, 1U,
                               N25Q_TEST_TIMEOUT_MS) != OK ||
            test_compare(locations[i], &test_value, 1U) != OK)
        {
            iprint("[FAIL] Bulk erase preparation\r\n");
            return ERR;
        }
    }

    if (n25q_erase_chip(N25Q_TEST_BULK_TIMEOUT_MS) != OK)
    {
        iprint("[FAIL] Bulk erase\r\n");
        return ERR;
    }

    for (uint32_t i = 0U; i < 3U; i++)
    {
        if (test_erased(locations[i], 1U) != OK)
        {
            iprint("[FAIL] Bulk erase spot check\r\n");
            return ERR;
        }
    }

    iprint("[PASS] Bulk erase spot check\r\n");


    iprint("[PASS] All enabled tests\r\n");
    return OK;
}


