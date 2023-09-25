/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/syscall_handler.h>

static inline int z_vrfy_comparator_configure(const struct device *dev,
					      const struct comparator_cfg *cfg)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COMPARATOR(dev, configure));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(cfg, sizeof(struct comparator_cfg)));
	return z_impl_comparator_configure(dev, cfg);
}
#include <syscalls/comparator_configure_mrsh.c>

static inline int z_vrfy_comparator_start(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COMPARATOR(dev, start));
	return z_impl_comparator_start(dev);
}
#include <syscalls/comparator_start_mrsh.c>

static inline int z_vrfy_comparator_stop(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COMPARATOR(dev, stop));
	return z_impl_comparator_stop(dev);
}
#include <syscalls/comparator_stop_mrsh.c>

static inline int z_vrfy_comparator_get_state(const struct device *dev,
					      uint32_t *state)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COMPARATOR(dev, get_state));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(state, sizeof(uint32_t)));
	return z_impl_comparator_get_state(dev, state);
}
#include <syscalls/comparator_get_state_mrsh.c>
