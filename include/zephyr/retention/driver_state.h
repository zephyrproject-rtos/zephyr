/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for retention driver state
 */

#ifndef ZEPHYR_INCLUDE_RETENTION_DRIVER_STATE_
#define ZEPHYR_INCLUDE_RETENTION_DRIVER_STATE_

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct retention_driver_state_header {
	/* driver state has been written at least once */
	uint8_t initialized: 1;
};

/**
 * @brief Check if the retention driver state size is sufficient.
 */
#define RETENTION_DRIVER_STATE_SIZE_CHECK(node_id, minimum_size)                                   \
	BUILD_ASSERT(DT_REG_SIZE(node_id) >=                                                       \
			     minimum_size + sizeof(struct retention_driver_state_header),          \
		     "size of driver state area is too small")

/**
 * @brief Retention Driver State API
 * @defgroup retention_driver_state_api Retention Driver State API
 * @since 3.6
 * @version 0.1.0
 * @ingroup os_services
 * @{
 */

typedef bool (*retention_driver_state_is_valid_api)(const struct device *dev);
typedef int (*retention_driver_state_read_api)(const struct device *dev, uint8_t *buffer,
					       size_t size);
typedef int (*retention_driver_state_write_api)(const struct device *dev, const uint8_t *buffer,
						size_t size);

struct retention_driver_state_api {
	retention_driver_state_is_valid_api is_valid;
	retention_driver_state_read_api read;
	retention_driver_state_write_api write;
};

/**
 * @brief		Checks if the underlying data in the retention area is valid or not.
 *
 * @param dev		Retention driver state device to use.
 *
 * @retval true		The driver state is valid and can be used.
 */
bool retention_driver_state_is_valid(const struct device *dev);

/**
 * @brief		Reads data from the driver state in the retention area.
 *
 * @param dev		Retention driver state device to use.
 * @param buffer	Buffer to store read data in.
 * @param size		Size of data to read.
 *
 * @retval 0		If successful.
 * @retval -errno	Error code code.
 */
int retention_driver_state_read(const struct device *dev, uint8_t *buffer, size_t size);

/**
 * @brief		Writes the driver state into the retention area.
 *
 * @param dev		Retention device to use.
 * @param buffer	Data to write.
 * @param size		Size of data to be written.
 *
 * @retval		0 on success else negative errno code.
 */
int retention_driver_state_write(const struct device *dev, const uint8_t *buffer, size_t size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RETENTION_DRIVER_STATE_ */
