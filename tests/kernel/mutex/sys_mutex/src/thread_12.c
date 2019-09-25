/* thread_12.c - helper file for testing kernel mutex APIs */

/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief mutex test helper
 *
 * This module defines a thread that is used in recursive mutex locking tests.
 * It helps ensure that a private mutex can be referenced in a file other than
 * the one it was defined in.
 */

#include <tc_util.h>
#include <zephyr.h>
#include <sys/mutex.h>

static int tc_rc = TC_PASS;         /* test case return code */

extern struct sys_mutex private_mutex;

/**
 *
 * thread_12 - thread that participates in recursive locking tests
 *
 * @return  N/A
 */

void thread_12(void)
{
	int rv;

	/* Wait for private mutex to be released */

	rv = sys_mutex_lock(&private_mutex, K_FOREVER);
	if (rv != 0) {
		tc_rc = TC_FAIL;
		TC_ERROR("Failed to obtain private mutex\n");
		return;
	}

	/* Wait a bit, then release the mutex */

	k_sleep(K_MSEC(500));
	sys_mutex_unlock(&private_mutex);

}
