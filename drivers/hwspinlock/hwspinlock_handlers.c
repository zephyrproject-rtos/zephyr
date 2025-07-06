/*
 * Copyright (c) 2023 Sequans Communications
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwspinlock.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_hwspinlock_trylock(const struct device *dev, uint32_t id)
{
	K_OOPS(K_SYSCALL_DRIVER_HWSPINLOCK(dev, trylock));
	return z_impl_hwspinlock_trylock(dev, id);
}

#include <zephyr/syscalls/hwspinlock_trylock_mrsh.c>

static inline void z_vrfy_hwspinlock_lock(const struct device *dev, uint32_t id)
{
	K_OOPS(K_SYSCALL_DRIVER_HWSPINLOCK(dev, lock));
	z_impl_hwspinlock_lock(dev, id);
}

#include <zephyr/syscalls/hwspinlock_lock_mrsh.c>

static inline void z_vrfy_hwspinlock_unlock(const struct device *dev, uint32_t id)
{
	K_OOPS(K_SYSCALL_DRIVER_HWSPINLOCK(dev, unlock));
	z_impl_hwspinlock_unlock(dev, id);
}

#include <zephyr/syscalls/hwspinlock_unlock_mrsh.c>

static inline uint32_t z_vrfy_hwspinlock_get_max_id(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_HWSPINLOCK(dev, get_max_id));
	return z_impl_hwspinlock_get_max_id(dev);
}

#include <zephyr/syscalls/hwspinlock_get_max_id_mrsh.c>
