/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_EMUL_BBRAM_H_
#define INCLUDE_ZEPHYR_DRIVERS_EMUL_BBRAM_H_

#include <zephyr/drivers/emul.h>

#include <stdint.h>

/**
 * @brief BBRAM emulator backend API
 * @defgroup bbram_emulator_backend BBRAM emulator backend API
 * @ingroup io_interfaces
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
 */

__subsystem struct emul_bbram_driver_api {
	/** Sets the data */
	int (*set_data)(const struct emul *target, size_t offset, size_t count,
			const uint8_t *data);
	/** Checks the data */
	int (*get_data)(const struct emul *target, size_t offset, size_t count, uint8_t *data);
};

/**
 * @endcond
 */

/**
 * @brief Set the expected data in the bbram region
 *
 * @param target Pointer to the emulator instance to operate on
 * @param offset Offset within the memory to set
 * @param count Number of bytes to write
 * @param data The data to write
 * @return 0 if successful
 * @return -ENOTSUP if no backend API or if the set_data function isn't implemented
 * @return -ERANGE if the destination address is out of range.
 */
static inline int emul_bbram_backend_set_data(const struct emul *target, size_t offset,
					      size_t count, const uint8_t *data)
{
	if (target == NULL || target->backend_api == NULL) {
		return -ENOTSUP;
	}

	struct emul_bbram_driver_api *api = (struct emul_bbram_driver_api *)target->backend_api;

	if (api->set_data == NULL) {
		return -ENOTSUP;
	}

	return api->set_data(target, offset, count, data);
}

/**
 * @brief Get the expected data in the bbram region
 *
 * @param target Pointer to the emulator instance to operate on
 * @param offset Offset within the memory to get
 * @param count Number of bytes to read
 * @param data The data buffer to hold the result
 * @return 0 if successful
 * @return -ENOTSUP if no backend API or if the get_data function isn't implemented
 * @return -ERANGE if the address is out of range.
 */
static inline int emul_bbram_backend_get_data(const struct emul *target, size_t offset,
					      size_t count, uint8_t *data)
{
	if (target == NULL || target->backend_api == NULL) {
		return -ENOTSUP;
	}

	struct emul_bbram_driver_api *api = (struct emul_bbram_driver_api *)target->backend_api;

	if (api->get_data == NULL) {
		return -ENOTSUP;
	}

	return api->get_data(target, offset, count, data);
}

/**
 * @}
 */

#endif /* INCLUDE_ZEPHYR_DRIVERS_EMUL_BBRAM_H_ */
