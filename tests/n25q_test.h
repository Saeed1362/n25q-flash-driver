/**
 * @file n25q_test.h
 * @brief Public interface for the N25Q flash functional tests.
 *
 * @date Nov 7, 2026
 * @author Saeed Ansari
 */

#ifndef INC_N25Q_TEST_H_
#define INC_N25Q_TEST_H_

#include "status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runs the complete enabled N25Q flash test sequence.
 *
 * Test results are reported through the configured UART output.
 *
 * @return OK when all enabled tests pass, otherwise ERR.
 */
STATUS n25q_test(void);

/**
 * @brief Transmits a null-terminated string through the test UART.
 *
 * @param str Pointer to the string to transmit. NULL and empty strings are ignored.
 */
void iprint(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* INC_N25Q_TEST_H_ */
