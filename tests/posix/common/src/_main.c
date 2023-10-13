/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

extern bool fdtable_fd_is_initialized(int fd);

static void *setup(void)
{
	/* ensure that the the stdin, stdout, and stderr fdtable entries are initialized */
	zassert_true(fdtable_fd_is_initialized(0));
	zassert_true(fdtable_fd_is_initialized(1));
	zassert_true(fdtable_fd_is_initialized(2));

	return NULL;
}

ZTEST_SUITE(posix_apis, NULL, setup, NULL, NULL, NULL);
