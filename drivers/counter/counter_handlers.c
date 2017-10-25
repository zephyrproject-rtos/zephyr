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
	_SYSCALL_HANDLER1_SIMPLE(name, K_OBJ_DRIVER_COUNTER, struct device *)

COUNTER_HANDLER(counter_get_pending_int);
COUNTER_HANDLER(counter_read);
COUNTER_HANDLER(counter_stop);
COUNTER_HANDLER(counter_start);
