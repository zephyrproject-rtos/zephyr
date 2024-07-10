/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for SYSCON driver APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SYSCON_H_
#error "Should only be included by zephyr/drivers/syscon.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_SYSCON_INTERNAL_SYSCON_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SYSCON_INTERNAL_SYSCON_IMPL_H_

#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_syscon_get_base(const struct device *dev, uintptr_t *addr)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOTSUP;
	}

	return api->get_base(dev, addr);
}

static inline int z_impl_syscon_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOTSUP;
	}

	return api->read(dev, reg, val);
}

static inline int z_impl_syscon_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	if (api == NULL) {
		return -ENOTSUP;
	}

	return api->write(dev, reg, val);
}

static inline int z_impl_syscon_get_size(const struct device *dev, size_t *size)
{
	const struct syscon_driver_api *api = (const struct syscon_driver_api *)dev->api;

	return api->get_size(dev, size);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SYSCON_INTERNAL_SYSCON_IMPL_H_ */
