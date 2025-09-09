/*
 * Copyright (c) 2023 Sequans Communications
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup hwspinlock_interface
 * @brief Main header file for hardware spinlock driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_
#define ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_

/**
 * @brief Interfaces for hardware spinlocks.
 * @defgroup hwspinlock_interface Hardware Spinlock
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Callback API for trying to lock HW spinlock
 * @see hwspinlock_trylock().
 */
typedef int (*hwspinlock_api_trylock)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to lock HW spinlock
 * @see hwspinlock_lock().
 */
typedef void (*hwspinlock_api_lock)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to unlock HW spinlock
 * @see hwspinlock_unlock().
 */
typedef void (*hwspinlock_api_unlock)(const struct device *dev, uint32_t id);

/**
 * @brief Callback API to get HW spinlock max ID
 * @see hwspinlock_get_max_id().
 */
typedef uint32_t (*hwspinlock_api_get_max_id)(const struct device *dev);

__subsystem struct hwspinlock_driver_api {
	hwspinlock_api_trylock trylock;
	hwspinlock_api_lock lock;
	hwspinlock_api_unlock unlock;
	hwspinlock_api_get_max_id get_max_id;
};
/**
 * @endcond
 */

/**
 * @brief Try to lock HW spinlock
 *
 * This function is used for try to lock specific HW spinlock. It should
 * be called before a critical section that we want to protect.
 *
 * @param dev HW spinlock device instance.
 * @param id  Spinlock identifier.
 *
 * @retval 0 If successful.
 * @retval -errno In case of any failure.
 */
__syscall int hwspinlock_trylock(const struct device *dev, uint32_t id);

static inline int z_impl_hwspinlock_trylock(const struct device *dev, uint32_t id)
{
	const struct hwspinlock_driver_api *api =
		(const struct hwspinlock_driver_api *)dev->api;

	if (api->trylock == NULL) {
		return -ENOSYS;
	}

	return api->trylock(dev, id);
}

/**
 * @brief Lock HW spinlock
 *
 * This function is used to lock specific HW spinlock. It should be
 * called before a critical section that we want to protect.
 *
 * @param dev HW spinlock device instance.
 * @param id  Spinlock identifier.
 */
__syscall void hwspinlock_lock(const struct device *dev, uint32_t id);

static inline void z_impl_hwspinlock_lock(const struct device *dev, uint32_t id)
{
	const struct hwspinlock_driver_api *api =
		(const struct hwspinlock_driver_api *)dev->api;

	if (api->lock != NULL) {
		api->lock(dev, id);
	}
}

/**
 * @brief Try to unlock HW spinlock
 *
 * This function is used for try to unlock specific HW spinlock. It should
 * be called after a critical section that we want to protect.
 *
 * @param dev HW spinlock device instance.
 * @param id  Spinlock identifier.
 */
__syscall void hwspinlock_unlock(const struct device *dev, uint32_t id);

static inline void z_impl_hwspinlock_unlock(const struct device *dev, uint32_t id)
{
	const struct hwspinlock_driver_api *api =
		(const struct hwspinlock_driver_api *)dev->api;

	if (api->unlock != NULL) {
		api->unlock(dev, id);
	}
}

/**
 * @brief Get HW spinlock max ID
 *
 * This function is used to get the HW spinlock maximum ID. It should
 * be called before attempting to lock/unlock a specific HW spinlock.
 *
 * @param dev HW spinlock device instance.
 *
 * @retval HW spinlock max ID.
 * @retval 0 if the function is not implemented by the driver.
 */
__syscall uint32_t hwspinlock_get_max_id(const struct device *dev);

static inline uint32_t z_impl_hwspinlock_get_max_id(const struct device *dev)
{
	const struct hwspinlock_driver_api *api =
		(const struct hwspinlock_driver_api *)dev->api;

	if (api->get_max_id == NULL) {
		return 0;
	}

	return api->get_max_id(dev);
}

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/syscalls/hwspinlock.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_ */
