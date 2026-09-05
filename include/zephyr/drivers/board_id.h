/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2026 The Zephyr Project Contributors
 * Copyright (c) 2026 Dev It Wise
 */

/**
 * @file
 * @brief Public API for board identity / revision drivers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BOARD_ID_H_
#define ZEPHYR_INCLUDE_DRIVERS_BOARD_ID_H_

#include <errno.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Board identity interface
 * @defgroup board_id_interface Board identity
 * @since 4.5
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Callback API for reading the board identity value.
 * See board_id_read() for argument descriptions.
 */
typedef ssize_t (*board_id_read_t)(const struct device *dev, uint8_t *buffer, size_t length);

/** @brief Board-id driver API. */
__subsystem struct board_id_driver_api {
	/** Read the raw board identity value. */
	board_id_read_t read;
};

/**
 * @brief Copy the raw board identity value into a buffer
 *
 * The value is the raw encoding on the board (strap bits, an EEPROM word,
 * ...); mapping it to a semantic meaning is application/board-integrator
 * policy, not this API's job.
 *
 * The identity is expressed as a big-endian (most significant byte first)
 * byte sequence, the same convention hwinfo_get_device_id() documents, so a
 * hex dump of the buffer reads as the intended value regardless of host
 * endianness.
 *
 * If @p length is smaller than the identity, only the leading @p length
 * bytes are written and that count is returned; the caller gets a short,
 * but never a wrong, value instead of a silently truncated one. If
 * @p length is larger than the identity, only the identity's own byte count
 * is written and returned; the rest of the buffer is left unchanged. This
 * mirrors hwinfo_get_device_id().
 *
 * A driver may cache the value at init instead of reading live; check its
 * own documentation for which.
 *
 * @param dev Board-id device instance.
 * @param[out] buffer Buffer to write the identity to.
 * @param length Size of @p buffer, in bytes.
 *
 * @return Number of bytes written on success, negative errno value on failure.
 * @retval -EINVAL If @p buffer is `NULL` or @p length is 0.
 * @retval -EIO If the underlying transport failed.
 */
__syscall ssize_t board_id_read(const struct device *dev, uint8_t *buffer, size_t length);

static inline ssize_t z_impl_board_id_read(const struct device *dev, uint8_t *buffer,
					    size_t length)
{
	const struct board_id_driver_api *api = DEVICE_API_GET(board_id, dev);

	if (buffer == NULL || length == 0U) {
		return -EINVAL;
	}

	return api->read(dev, buffer, length);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/board_id.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_BOARD_ID_H_ */
