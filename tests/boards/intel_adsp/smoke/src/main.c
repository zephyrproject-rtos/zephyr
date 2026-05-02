/*
 * Copyright (c) 2022, 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdlib.h>

#if XCHAL_HAVE_VECBASE
ZTEST(intel_adsp, test_vecbase_lock)
{
	uintptr_t vecbase;

	/* Unfortunately there is not symbol to check if the target
	 * supports locking VECBASE. The best we can do is checking if
	 * the first is not set and skip the test.
	 */
	__asm__ volatile("rsr.vecbase %0" : "=r"(vecbase));
	if ((vecbase & 0x1) == 0) {
		ztest_test_skip();
	}

	/* VECBASE register should have been locked during the cpu
	 * start up. Trying to change its location should fail.
	 */
	__asm__ volatile("wsr.vecbase %0; rsync" : : "r"(0x0));
	__asm__ volatile("rsr.vecbase %0" : "=r"(vecbase));

	zassert_not_equal(vecbase, 0x0, "VECBASE was changed");
}
#endif

static void intel_adsp_teardown(void *data)
{
	/* Wait a bit so the python script on host is ready to receive
	 * IPC messages. An IPC message could be used instead of a timer,
	 * but expecting IPC to be working on a test suite that is going
	 * to test IPC may not be indicated.
	 */
	k_msleep(1000);
}

ZTEST_SUITE(intel_adsp, NULL, NULL, NULL, NULL, intel_adsp_teardown);

ZTEST_SUITE(intel_adsp_boot, NULL, NULL, NULL, NULL, NULL);
