/*
 * Copyright (c) 2024 VCI Development
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWINFO_LPC_H_
#define ZEPHYR_INCLUDE_DRIVERS_HWINFO_LPC_H_

#include <zephyr/types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the LPC54S018 unique device ID
 *
 * The LPC54S018 provides a 128-bit unique ID that is programmed
 * during manufacturing and cannot be changed.
 *
 * @param buffer Pointer to buffer to store the unique ID
 * @param length Length of the buffer (must be at least 16 bytes)
 *
 * @retval 0 on success
 * @retval -EINVAL if buffer is NULL or length < 16
 * @retval -ENOTSUP if unique ID is not available
 */
int lpc_get_unique_id(uint8_t *buffer, size_t length);

/**
 * @brief Get the device serial number as a string
 *
 * Converts the unique ID to a hexadecimal string representation.
 *
 * @param serial Buffer to store the serial number string
 * @param length Length of the buffer (must be at least 33 bytes for null terminator)
 *
 * @retval 0 on success
 * @retval -EINVAL if buffer is NULL or length < 33
 */
int lpc_get_serial_number(char *serial, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_HWINFO_LPC_H_ */