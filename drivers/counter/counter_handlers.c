/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/counter.h>

/* For those APIs that just take one argument which is a counter driver
 * instance and return an integral value
 */
#define COUNTER_HANDLER(name) \
	static inline int z_vrfy_counter_##name(struct devince *dev) \
	{ \
		Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, name)); \
		return z_impl_counter_ ## name((struct device *)dev); \
	}

COUNTER_HANDLER(get_pending_int)
COUNTER_HANDLER(read)
COUNTER_HANDLER(stop)
COUNTER_HANDLER(start)
COUNTER_HANDLER(get_top_value)
COUNTER_HANDLER(get_max_relative_alarm)

#include <syscalls/counter_get_pending_int_mrsh.c>
#include <syscalls/counter_read_mrsh.c>
#include <syscalls/counter_stop_mrsh.c>
#include <syscalls/counter_start_mrsh.c>
#include <syscalls/counter_get_top_value_mrsh.c>
#include <syscalls/counter_get_max_relative_alarm_mrsh.c>

static inline u32_t z_vrfy_counter_get_guard_period(struct device *dev,
							u32_t flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, get_guard_period));
	return z_impl_counter_get_guard_period((struct device *)dev, flags);
}
#include <syscalls/counter_get_guard_period_mrsh.c>

static inline int z_vrfy_counter_set_guard_period(struct device *dev,
						   u32_t ticks, u32_t flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, set_guard_period));
	return z_impl_counter_set_guard_period((struct device *)dev, ticks,
						flags);
}
#include <syscalls/counter_set_guard_period_mrsh.c>
