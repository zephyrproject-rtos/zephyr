/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "test_common.h"

#ifndef CONFIG_ARC_DSP
#error Rebuild with the ARC_DSP config option enabled
#endif

#ifndef CONFIG_ARC_DSP_SHARING
#error Rebuild with the ARC_DSP_SHARING config option enabled
#endif

static void *generic_setup(void)
{
	/*
	 * Enable round robin scheduling to allow both the low priority complex
	 * computation and load/store tasks to execute. The high priority complex
	 * computation and load/store tasks will preempt the low priority tasks
	 * periodically.
	 */
	k_sched_time_slice_set(10, THREAD_LOW_PRIORITY);

	return NULL;
}

ZTEST_SUITE(dsp_sharing, NULL, generic_setup, NULL, NULL, NULL);
