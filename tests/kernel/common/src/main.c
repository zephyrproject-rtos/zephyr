/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/ztest.h>
#include <zephyr/kernel_version.h>
#include <zephyr/sys/speculation.h>
#include <zephyr/version.h>

/**
 * @defgroup kernel_common_tests Common Tests
 * @ingroup all_tests
 * @{
 * @}
 *
 */

#ifndef CONFIG_PRINTK
/**
 * @brief Skipped stub of the printk test used when CONFIG_PRINTK is disabled.
 *
 * @ingroup kernel_printk_tests
 */
ZTEST(printk, test_printk)
{
	ztest_test_skip();
}
#endif

/**
 * @brief Verify sys_kernel_version_get() reports the build-time kernel version.
 *
 * @ingroup kernel_common_tests
 *
 * @details
 * Passing proves that the runtime kernel version returned by
 * sys_kernel_version_get() matches the version constants the image was built
 * with, so version reporting stays consistent with the source tree.
 *
 * Test steps:
 * - Retrieve the packed kernel version via sys_kernel_version_get().
 * - Extract the major, minor, and patchlevel fields with the SYS_KERNEL_VER_*
 *   accessor macros.
 *
 * Expected result:
 * - Each extracted field equals its corresponding KERNEL_VERSION_* /
 *   KERNEL_PATCHLEVEL build-time constant.
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

/**
 * @brief Verify k_array_index_sanitize() clamps out-of-bounds indices.
 *
 * @ingroup kernel_common_tests
 *
 * @details
 * Passing proves that the speculation-mitigation helper returns an in-bounds
 * index unchanged and, under userspace, sanitizes an out-of-bounds index to a
 * safe value (0) so that speculative out-of-bounds array accesses cannot leak
 * data.
 *
 * Test steps:
 * - Sanitize an in-bounds index (17 against bound 24) and check it is unchanged.
 * - Under CONFIG_USERSPACE, sanitize an out-of-bounds index (17 against bound 5)
 *   and check the result.
 *
 * Expected result:
 * - The in-bounds index is returned as-is (17); the out-of-bounds index is
 *   clamped to 0 when userspace is enabled.
 *
 * @see k_array_index_sanitize()
 */
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

void *common_setup(void)
{
#if CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &eno_thread, &eno_stack);
#endif

	return NULL;
}

ZTEST_SUITE(common, NULL, common_setup, NULL, NULL, NULL);
