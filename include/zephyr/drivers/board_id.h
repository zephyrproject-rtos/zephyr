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

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Board identity interface
 * @defgroup board_id_interface Board identity
 * @since 4.3
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief Callback API for reading the board identity value.
 * See board_id_read() for argument descriptions.
 */
typedef int (*board_id_read_t)(const struct device *dev, uint32_t *id);

/** @brief Board-id driver API. */
__subsystem struct board_id_driver_api {
	/** Read the raw board identity value. */
	board_id_read_t read;
};

/**
 * @brief Read the raw board identity value.
 *
 * The value is the raw encoding the board carries (strap bits, an EEPROM
 * word, ...). Mapping it to a semantic meaning ("v3.2 with the radio
 * populated") is application or board-integrator policy and is deliberately
 * outside this API.
 *
 * A driver may read the value once at init and always return that cached
 * value; consult the driver's own documentation to know whether a given
 * backend reads live or returns a cached value.
 *
 * @param dev Board-id device instance.
 * @param[out] id Raw board identity value. Left untouched on error.
 *
 * @retval 0 On success.
 * @retval -EIO If the underlying transport failed.
 * @retval -EINVAL If @p id is `NULL`.
 */
__syscall int board_id_read(const struct device *dev, uint32_t *id);

static inline int z_impl_board_id_read(const struct device *dev, uint32_t *id)
{
	const struct board_id_driver_api *api = DEVICE_API_GET(board_id, dev);

	if (id == NULL) {
		return -EINVAL;
	}

	return api->read(dev, id);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/board_id.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_BOARD_ID_H_ */
