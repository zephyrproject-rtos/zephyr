/**
 * @file
 *
 * @brief Public APIs to get device Information.
 */

/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWINFO_H_
#define ZEPHYR_INCLUDE_DRIVERS_HWINFO_H_

#include <zephyr/types.h>
#include <sys/types.h>
#include <stddef.h>
#include <errno.h>
#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Device ID
 * @{
 */


/**
 * @brief Copy the device id to a buffer
 *
 * This routine copies "length" number of bytes of the device ID to the buffer.
 * If the device ID is smaller then length, the rest of the buffer is left unchanged.
 * The ID depends on the hardware and is not guaranteed unique.
 *
 * @param buffer  Buffer to write the ID to.
 * @param length  Max length of the buffer.
 *
 * @retval size of the device ID copied or negative on error.
 */
__syscall ssize_t hwinfo_get_device_id(u8_t *buffer, size_t length);

ssize_t z_impl_hwinfo_get_device_id(u8_t *buffer, size_t length);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/hwinfo.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HWINFO_H_ */
