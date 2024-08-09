/*
 * Copyright (c) 2023 Sequans Communications
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_
#error "Should only be included by zephyr/drivers/hwspinlock.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_INTERNAL_HWSPINLOCK_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_INTERNAL_HWSPINLOCK_IMPL_H_

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_hwspinlock_trylock(const struct device *dev, uint32_t id)
{
	const struct hwspinlock_driver_api *api =
		(const struct hwspinlock_driver_api *)dev->api;

	if (api->trylock == NULL)
		return -ENOSYS;

	return api->trylock(dev, id);
}

static inline void z_impl_hwspinlock_lock(const struct device *dev, uint32_t id)
{
	const struct hwspinlock_driver_api *api =
		(const struct hwspinlock_driver_api *)dev->api;

	if (api->lock != NULL)
		api->lock(dev, id);
}

static inline void z_impl_hwspinlock_unlock(const struct device *dev, uint32_t id)
{
	const struct hwspinlock_driver_api *api =
		(const struct hwspinlock_driver_api *)dev->api;

	if (api->unlock != NULL)
		api->unlock(dev, id);
}

static inline uint32_t z_impl_hwspinlock_get_max_id(const struct device *dev)
{
	const struct hwspinlock_driver_api *api =
		(const struct hwspinlock_driver_api *)dev->api;

	if (api->get_max_id == NULL)
		return 0;

	return api->get_max_id(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_INTERNAL_HWSPINLOCK_IMPL_H_ */
