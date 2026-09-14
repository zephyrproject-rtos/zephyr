/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

K_SEM_DEFINE(obj_access, 0, 1);
K_SEM_DEFINE(obj_no_access, 0, 1);
int no_obj;

/**
 * @brief Verify that k_object_access_check() reports the caller's rights.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The check call lets a user thread ask, without side effects, whether it may
 * use an object. Each answer must match the actual state: granted object,
 * object never granted, address that is no kernel object, and the thread's
 * own thread object, which every thread implicitly owns.
 *
 * Test steps:
 * - Check a semaphore granted during suite setup.
 * - Check a semaphore that was never granted.
 * - Check an address that is not a kernel object.
 * - Check the calling thread's own thread object.
 *
 * Expected result:
 * - The calls return 0, -EPERM, -EBADF and 0 respectively.
 *
 * @see k_object_access_check()
 */
ZTEST_USER(userspace_access_check, test_kobject_access_check)
{
	zexpect_equal(k_object_access_check(&obj_access), 0, "should have access but doesn't");
	zexpect_equal(k_object_access_check(&obj_no_access), -EPERM,
		      "should not have access but does");
	zexpect_equal(k_object_access_check(&no_obj), -EBADF, "should not be valid object but is");

	/* User thread should always have access to itself */
	zexpect_equal(k_object_access_check(k_current_get()), 0, "no access to itself");
}

static void *userspace_access_setup(void)
{
	k_object_access_grant(&obj_access, k_current_get());

	return NULL;
}

ZTEST_SUITE(userspace_access_check, NULL, userspace_access_setup, NULL, NULL, NULL);
