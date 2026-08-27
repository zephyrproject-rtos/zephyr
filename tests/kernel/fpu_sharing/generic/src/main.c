/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "test_common.h"

#ifndef CONFIG_FPU
#error Rebuild with the FPU config option enabled
#endif

#ifndef CONFIG_FPU_SHARING
#error Rebuild with the FPU_SHARING config option enabled
#endif

static void *generic_setup(void)
{
	/*
	 * Enable round robin scheduling to allow both the low priority pi
	 * computation and load/store tasks to execute. The high priority pi
	 * computation and load/store tasks will preempt the low priority tasks
	 * periodically.
	 */
	k_sched_time_slice_set(10, THREAD_LOW_PRIORITY);

	return NULL;
}

ZTEST_SUITE(fpu_sharing_generic, NULL, generic_setup, NULL, NULL, NULL);
