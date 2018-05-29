/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <counter.h>

/* For those APIs that just take one argument which is a counter driver
 * instance and return an integral value
 */
#define COUNTER_HANDLER(name) \
	Z_SYSCALL_HANDLER(counter_ ## name, dev) \
	{ \
		Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, name)); \
		return _impl_counter_ ## name((struct device *)dev); \
	}

COUNTER_HANDLER(get_pending_int)
COUNTER_HANDLER(read)
COUNTER_HANDLER(stop)
COUNTER_HANDLER(start)
