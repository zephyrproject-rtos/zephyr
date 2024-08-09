/*
 * Copyright (c) 2023 Sequans Communications
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_
#define ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_

/**
 * @brief HW spinlock Interface
 * @defgroup hwspinlock_interface HW spinlock Interface
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

#ifdef __cplusplus
}
#endif

/** @} */

#include <zephyr/drivers/hwspinlock/internal/hwspinlock_impl.h>
#include <zephyr/syscalls/hwspinlock.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_HWSPINLOCK_H_ */
