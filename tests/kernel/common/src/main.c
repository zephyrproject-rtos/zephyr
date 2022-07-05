/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>
#include <zephyr/kernel_version.h>
#include <zephyr/sys/speculation.h>
#include "version.h"

/**
 * @defgroup kernel_common_tests Common Tests
 * @ingroup all_tests
 * @{
 * @}
 *
 */

#ifdef CONFIG_ARM
ZTEST(bitfield, test_bitfield)
{
	ztest_test_skip();
}
#endif

#ifndef CONFIG_PRINTK
ZTEST(printk, test_printk)
{
	ztest_test_skip();
}
#endif

/**
 * @brief Test sys_kernel_version_get() functionality
 *
 * @ingroup kernel_common_tests
 *
 * @see sys_kernel_version_get()
 */
ZTEST(common, test_version)
{
	uint32_t version = sys_kernel_version_get();

	zassert_true(SYS_KERNEL_VER_MAJOR(version) == KERNEL_VERSION_MAJOR,
		     "major version mismatch");
	zassert_true(SYS_KERNEL_VER_MINOR(version) == KERNEL_VERSION_MINOR,
		     "minor version mismatch");
	zassert_true(SYS_KERNEL_VER_PATCHLEVEL(version) == KERNEL_PATCHLEVEL,
		     "patchlevel version match");

}

ZTEST(common, test_bounds_check_mitigation)
{
	/* Very hard to test against speculation attacks, but we can
	 * at least assert that logically this function does
	 * what it says it does.
	 */

	int index = 17;

	index = k_array_index_sanitize(index, 24);
	zassert_equal(index, 17, "bad index");

#ifdef CONFIG_USERSPACE
	index = k_array_index_sanitize(index, 5);
	zassert_equal(index, 0, "bad index");
#endif
}

extern struct k_stack eno_stack;
extern struct k_thread eno_thread;

static void *common_setup(void)
{
#if CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &eno_thread, &eno_stack);
#endif

	return NULL;
}

ZTEST_SUITE(common, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(atomic, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(bitarray, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(bitfield, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(boot_delay, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(byteorder, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(clock, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(common_errno, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(irq_offload, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(multilib, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(pow2, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(printk, NULL, common_setup, NULL, NULL, NULL);

ZTEST_SUITE(common_1cpu, NULL, common_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
