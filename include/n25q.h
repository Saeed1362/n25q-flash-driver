/**
 * @file n25q.h
 * @brief Public API for the N25Q serial flash memory driver.
 *
 * This header defines supported memory commands, device capacities, geometry,
 * status values, and functions for reading, writing, and erasing N25Q devices.
 *
 * @date Nov 1, 2026
 * @author Saeed Ansari
 */

#ifndef INC_N25Q_H_
#define INC_N25Q_H_

#include <stdint.h>
#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ID                     0x9FU
#define DUMMY                  0xFFU
#define READ_STATUS            0x05U
#define READ_FLAG_STATUS       0x70U
#define CLEAR_FLAG_STATUS 	   0x50U
#define WRITE_STATUS           0x01U
#define PAGE_PROGRAM           0x02U
#define READ                   0x03U
#define WRITE_ENABLE           0x06U
#define SUBSECTOR_ERASE        0x20U
#define SECTOR_ERASE           0xD8U
#define BULK_ERASE             0xC7U


#define N25Q_PAGE_SIZE         256U
#define N25Q_SUBSECTOR_SIZE    4096U
#define N25Q_SECTOR_SIZE       65536U

/* Select exactly ONE device. */
// #define N25Q008
// #define N25Q016
// #define N25Q032
// #define N25Q064
#define N25Q128
// #define N25Q256
// #define N25Q512
// #define N25Q00A

#if (defined(N25Q008) + defined(N25Q016) + \
     defined(N25Q032) + defined(N25Q064) + \
     defined(N25Q128) + defined(N25Q256) + \
     defined(N25Q512) + defined(N25Q00A)) != 1
#error "Select exactly one N25Q device"
#endif

#if defined(N25Q008)
#define N25Q_CAPACITY_BYTES   1048576U    /* 1 MiB */

#elif defined(N25Q016)
#define N25Q_CAPACITY_BYTES   2097152U    /* 2 MiB */

#elif defined(N25Q032)
#define N25Q_CAPACITY_BYTES   4194304U    /* 4 MiB */

#elif defined(N25Q064)
#define N25Q_CAPACITY_BYTES   8388608U    /* 8 MiB */

#elif defined(N25Q128)
#define N25Q_CAPACITY_BYTES   16777216U   /* 16 MiB */

#elif defined(N25Q256)
#define N25Q_CAPACITY_BYTES   33554432U   /* 32 MiB */

#elif defined(N25Q512)
#define N25Q_CAPACITY_BYTES   67108864U   /* 64 MiB */

#elif defined(N25Q00A)
#define N25Q_CAPACITY_BYTES   134217728U  /* 128 MiB */
#endif


/** @brief Memory geometry information for the selected N25Q device. */
typedef struct
{
    uint32_t capacity_bytes; /**< Total flash capacity in bytes. */
    uint32_t page_size;      /**< Page size in bytes. */
    uint32_t subsector_size; /**< Subsector size in bytes. */
    uint32_t sector_size;    /**< Sector size in bytes. */
} N25Q_Info;

/** @brief Reads the three-byte N25Q device ID. @param[out] id Buffer for the ID. @return OK on success, otherwise ERR. */
STATUS n25q_read_id(uint8_t *id);

/** @brief Initializes and validates the N25Q interface. @return OK on success, otherwise ERR. */
STATUS n25q_init(void);

/** @brief Reads a status register. @param commandT Register read command. @param[out] status Receives the register value. @return OK on success, otherwise ERR. */
STATUS n25q_read_register(uint8_t commandT, uint8_t *status);

/** @brief Waits for the flash write-in-progress bit to clear. @param timeout Maximum wait in milliseconds. @return OK when ready, otherwise ERR. */
STATUS n25q_wait_ready(uint32_t timeout);

/** @brief Clears the flag status register. @return OK on success, otherwise ERR. */
STATUS n25q_clear_flag_status(void);

/** @brief Enables writes and waits for write-enable confirmation. @param timeout Maximum wait in milliseconds. @return OK on success, otherwise ERR. */
STATUS n25q_write_enable(uint32_t timeout);

/** @brief Reads one byte from a flash address. @param address 24-bit memory address. @param[out] data Receives the byte. @return OK on success, otherwise ERR. */
STATUS n25q_read(uint32_t address, uint8_t *data);

/** @brief Programs a range contained within one page. @param address Starting address. @param[in] data Data to program. @param length Number of bytes. @param timeout_ms Operation timeout in milliseconds. @return OK on success, otherwise ERR. */
STATUS n25q_page_program(uint32_t address, const uint8_t *data, uint32_t length, uint32_t timeout_ms);

/** @brief Writes data across one or more pages. @param address Starting address. @param[in] data Data to write. @param length Number of bytes. @param timeout_ms Operation timeout in milliseconds. @return OK on success, otherwise ERR. */
STATUS n25q_write(uint32_t address, const uint8_t *data, uint32_t length, uint32_t timeout_ms);

/** @brief Erases one subsector. @param address Address within or at the start of the subsector. @param timeout_ms Operation timeout in milliseconds. @return OK on success, otherwise ERR. */
STATUS n25q_erase_subsector(uint32_t address, uint32_t timeout_ms);

/** @brief Erases one sector. @param address Address within or at the start of the sector. @param timeout_ms Operation timeout in milliseconds. @return OK on success, otherwise ERR. */
STATUS n25q_erase_sector(uint32_t address, uint32_t timeout_ms);

/** @brief Erases the entire flash device. @param timeout_ms Operation timeout in milliseconds. @return OK on success, otherwise ERR. */
STATUS n25q_erase_chip(uint32_t timeout_ms);

/** @brief Gets the selected device memory geometry. @param[out] info Receives memory geometry. @return OK on success, otherwise ERR. */
STATUS n25q_get_info(N25Q_Info *info);

#ifdef __cplusplus
}
#endif

#endif /* INC_N25Q_H_ */
