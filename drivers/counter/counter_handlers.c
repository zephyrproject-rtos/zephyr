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
	Z_SYSCALL_HANDLER(counter_ ## name, dev) \
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

Z_SYSCALL_HANDLER(counter_get_guard_period, dev, flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, get_guard_period));
	return z_impl_counter_get_guard_period((struct device *)dev, flags);
}

Z_SYSCALL_HANDLER(counter_set_guard_period, dev, ticks, flags)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, set_guard_period));
	return z_impl_counter_set_guard_period((struct device *)dev, ticks,
						flags);
}
