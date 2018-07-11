/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>
#include <kernel_version.h>
#include "version.h"

extern void test_byteorder_memcpy_swap(void);
extern void test_byteorder_mem_swap(void);
extern void test_atomic(void);
extern void test_intmath(void);
extern void test_printk(void);
extern void test_slist(void);
extern void test_dlist(void);
extern void test_timeout_order(void);
extern void test_clock_cycle(void);
extern void test_clock_uptime(void);
extern void test_multilib(void);

/**
 * @defgroup kernel_common_tests Common Tests
 * @ingroup all_tests
 * @{
 * @}
 *
 */

#ifdef CONFIG_ARM
void test_bitfield(void)
{
	ztest_test_skip();
}
#else
extern void test_bitfield(void);
#endif

#ifndef CONFIG_PRINTK
void test_printk(void)
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
static void test_version(void)
{
	u32_t version = sys_kernel_version_get();

	zassert_true(SYS_KERNEL_VER_MAJOR(version) == KERNEL_VERSION_MAJOR,
		     "major version mismatch");
	zassert_true(SYS_KERNEL_VER_MINOR(version) == KERNEL_VERSION_MINOR,
		     "minor version mismatch");
	zassert_true(SYS_KERNEL_VER_PATCHLEVEL(version) == KERNEL_PATCHLEVEL,
		     "patchlevel version match");

}

void test_main(void)
{
	ztest_test_suite(common,
			 ztest_unit_test(test_byteorder_memcpy_swap),
			 ztest_unit_test(test_byteorder_mem_swap),
			 ztest_unit_test(test_atomic),
			 ztest_unit_test(test_bitfield),
			 ztest_unit_test(test_printk),
			 ztest_unit_test(test_slist),
			 ztest_unit_test(test_dlist),
			 ztest_unit_test(test_intmath),
			 ztest_unit_test(test_timeout_order),
			 ztest_unit_test(test_clock_uptime),
			 ztest_unit_test(test_clock_cycle),
			 ztest_unit_test(test_version),
			 ztest_unit_test(test_multilib)
			 );

	ztest_run_test_suite(common);
}
