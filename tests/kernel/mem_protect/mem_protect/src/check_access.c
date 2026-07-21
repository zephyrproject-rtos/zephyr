/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

K_SEM_DEFINE(obj_access, 0, 1);
K_SEM_DEFINE(obj_no_access, 0, 1);
int no_obj;

ZTEST_USER(userspace_access_check, test_access)
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
