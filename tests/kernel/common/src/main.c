/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>
#include <kernel_version.h>
#include "version.h"

extern void byteorder_test_memcpy_swap(void);
extern void byteorder_test_mem_swap(void);
extern void atomic_test(void);
extern void bitfield_test(void);
extern void intmath_test(void);
extern void printk_test(void);
extern void ring_buffer_test(void);
extern void slist_test(void);
extern void dlist_test(void);
extern void rand32_test(void);
extern void timeout_order_test(void);
extern void clock_test(void);


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
	ztest_test_suite(common_test,
			 ztest_unit_test(byteorder_test_memcpy_swap),
			 ztest_unit_test(byteorder_test_mem_swap),
			 ztest_unit_test(atomic_test),
#ifdef CONFIG_PRINTK
			 ztest_unit_test(printk_test),
#endif
			 ztest_unit_test(ring_buffer_test),
			 ztest_unit_test(slist_test),
			 ztest_unit_test(dlist_test),
			 ztest_unit_test(rand32_test),
			 ztest_unit_test(intmath_test),
			 ztest_unit_test(timeout_order_test),
			 ztest_unit_test(clock_test),
			 ztest_unit_test(test_version)
			 );

	ztest_run_test_suite(common_test);
}
