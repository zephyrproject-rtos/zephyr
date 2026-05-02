/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/comparator.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_comparator_get_output(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_COMPARATOR(dev, get_output));
	return z_impl_comparator_get_output(dev);
}
#include <zephyr/syscalls/comparator_get_output_mrsh.c>

static inline int z_vrfy_comparator_set_trigger(const struct device *dev,
						enum comparator_trigger trigger)
{
	K_OOPS(K_SYSCALL_DRIVER_COMPARATOR(dev, set_trigger));
	return z_impl_comparator_set_trigger(dev, trigger);
}
#include <zephyr/syscalls/comparator_set_trigger_mrsh.c>

static inline int z_vrfy_comparator_trigger_is_pending(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_COMPARATOR(dev, trigger_is_pending));
	return z_impl_comparator_trigger_is_pending(dev);
}
#include <zephyr/syscalls/comparator_trigger_is_pending_mrsh.c>
