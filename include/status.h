/**
 * @file status.h
 * @brief Common return-status type for the N25Q software modules.
 *
 * @date Nov 5, 2026
 * @author Saeed Ansari
 */

#ifndef INC_STATUS_H_
#define INC_STATUS_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Operation result status. */
typedef enum
{
    OK,  /**< The operation completed successfully. */
    ERR  /**< The operation failed or an argument was invalid. */
} STATUS;

#ifdef __cplusplus
}
#endif

#endif /* INC_STATUS_H_ */
