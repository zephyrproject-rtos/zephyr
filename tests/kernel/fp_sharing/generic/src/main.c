/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include "test_common.h"

#ifndef CONFIG_FLOAT
#error Rebuild with the FLOAT config option enabled
#endif

#ifndef CONFIG_FP_SHARING
#error Rebuild with the FP_SHARING config option enabled
#endif

#if defined(CONFIG_X86) && !defined(CONFIG_SSE)
#error Rebuild with the SSE config option enabled
#endif

extern void test_load_store(void);
extern void test_pi(void);

void test_main(void)
{
	/*
	 * Enable round robin scheduling to allow both the low priority pi
	 * computation and load/store tasks to execute. The high priority pi
	 * computation and load/store tasks will preempt the low priority tasks
	 * periodically.
	 */
	k_sched_time_slice_set(10, THREAD_LOW_PRIORITY);

	/* Run the testsuite */
	ztest_test_suite(fp_sharing,
			 ztest_unit_test(test_load_store),
			 ztest_unit_test(test_pi));
	ztest_run_test_suite(fp_sharing);
}
