/*
 * Copyright (c) 2022 Andrei-Edward Popa <andrei.popa105@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for Reset Controller driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RESET_H_
#error "Should only be included by zephyr/drivers/reset.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_RESET_INTERNAL_RESET_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_RESET_INTERNAL_RESET_IMPL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_reset_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	const struct reset_driver_api *api = (const struct reset_driver_api *)dev->api;

	if (api->status == NULL) {
		return -ENOSYS;
	}

	return api->status(dev, id, status);
}

static inline int z_impl_reset_line_assert(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = (const struct reset_driver_api *)dev->api;

	if (api->line_assert == NULL) {
		return -ENOSYS;
	}

	return api->line_assert(dev, id);
}

static inline int z_impl_reset_line_deassert(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = (const struct reset_driver_api *)dev->api;

	if (api->line_deassert == NULL) {
		return -ENOSYS;
	}

	return api->line_deassert(dev, id);
}

static inline int z_impl_reset_line_toggle(const struct device *dev, uint32_t id)
{
	const struct reset_driver_api *api = (const struct reset_driver_api *)dev->api;

	if (api->line_toggle == NULL) {
		return -ENOSYS;
	}

	return api->line_toggle(dev, id);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RESET_INTERNAL_RESET_IMPL_H_ */
