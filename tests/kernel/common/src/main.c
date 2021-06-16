/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <ztest.h>
#include <kernel_version.h>
#include <sys/speculation.h>
#include "version.h"

extern void test_byteorder_memcpy_swap(void);
extern void test_byteorder_mem_swap(void);
extern void test_sys_get_be64(void);
extern void test_sys_put_be64(void);
extern void test_sys_get_be48(void);
extern void test_sys_put_be48(void);
extern void test_sys_get_be32(void);
extern void test_sys_put_be32(void);
extern void test_sys_get_be24(void);
extern void test_sys_put_be24(void);
extern void test_sys_get_be16(void);
extern void test_sys_put_be16(void);
extern void test_sys_get_le16(void);
extern void test_sys_put_le16(void);
extern void test_sys_get_le24(void);
extern void test_sys_put_le24(void);
extern void test_sys_get_le32(void);
extern void test_sys_put_le32(void);
extern void test_sys_get_le48(void);
extern void test_sys_put_le48(void);
extern void test_sys_get_le64(void);
extern void test_sys_put_le64(void);
extern void test_atomic(void);
extern void test_threads_access_atomic(void);
extern void test_errno(void);
extern void test_printk(void);
extern void test_timeout_order(void);
extern void test_clock_cycle(void);
extern void test_clock_uptime(void);
extern void test_ms_time_duration(void);
extern void test_multilib(void);
extern void test_thread_context(void);
extern void test_bootdelay(void);
extern void test_irq_offload(void);
extern void test_bitarray_declare(void);
extern void test_bitarray_set_clear(void);
extern void test_bitarray_alloc_free(void);
extern void test_bitarray_region_set_clear(void);
extern void test_nop(void);

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
	uint32_t version = sys_kernel_version_get();

	zassert_true(SYS_KERNEL_VER_MAJOR(version) == KERNEL_VERSION_MAJOR,
		     "major version mismatch");
	zassert_true(SYS_KERNEL_VER_MINOR(version) == KERNEL_VERSION_MINOR,
		     "minor version mismatch");
	zassert_true(SYS_KERNEL_VER_PATCHLEVEL(version) == KERNEL_PATCHLEVEL,
		     "patchlevel version match");

}

static void test_bounds_check_mitigation(void)
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

void test_main(void)
{
#if CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &eno_thread, &eno_stack);
#endif
	ztest_test_suite(common,
			 ztest_unit_test(test_bootdelay),
			 ztest_unit_test(test_irq_offload),
			 ztest_unit_test(test_byteorder_memcpy_swap),
			 ztest_unit_test(test_byteorder_mem_swap),
			 ztest_unit_test(test_sys_get_be64),
			 ztest_unit_test(test_sys_put_be64),
			 ztest_unit_test(test_sys_get_be48),
			 ztest_unit_test(test_sys_put_be48),
			 ztest_unit_test(test_sys_get_be32),
			 ztest_unit_test(test_sys_put_be32),
			 ztest_unit_test(test_sys_get_be24),
			 ztest_unit_test(test_sys_put_be24),
			 ztest_unit_test(test_sys_get_be16),
			 ztest_unit_test(test_sys_put_be16),
			 ztest_unit_test(test_sys_get_le16),
			 ztest_unit_test(test_sys_put_le16),
			 ztest_unit_test(test_sys_get_le24),
			 ztest_unit_test(test_sys_put_le24),
			 ztest_unit_test(test_sys_get_le32),
			 ztest_unit_test(test_sys_put_le32),
			 ztest_unit_test(test_sys_get_le48),
			 ztest_unit_test(test_sys_put_le48),
			 ztest_unit_test(test_sys_get_le64),
			 ztest_unit_test(test_sys_put_le64),
			 ztest_user_unit_test(test_atomic),
			 ztest_unit_test(test_threads_access_atomic),
			 ztest_unit_test(test_bitfield),
			 ztest_unit_test(test_bitarray_declare),
			 ztest_unit_test(test_bitarray_set_clear),
			 ztest_unit_test(test_bitarray_alloc_free),
			 ztest_unit_test(test_bitarray_region_set_clear),
			 ztest_unit_test(test_printk),
			 ztest_1cpu_unit_test(test_timeout_order),
			 ztest_user_unit_test(test_clock_uptime),
			 ztest_unit_test(test_clock_cycle),
			 ztest_unit_test(test_version),
			 ztest_unit_test(test_multilib),
			 ztest_unit_test(test_thread_context),
			 ztest_user_unit_test(test_errno),
			 ztest_unit_test(test_ms_time_duration),
			 ztest_unit_test(test_bounds_check_mitigation),
			 ztest_unit_test(test_nop)
			 );

	ztest_run_test_suite(common);
}
