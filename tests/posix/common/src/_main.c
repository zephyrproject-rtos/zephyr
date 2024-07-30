/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

extern bool fdtable_fd_is_initialized(int fd);

ZTEST(posix_apis, test_fdtable_init)
{
	/* ensure that the stdin, stdout, and stderr fdtable entries are initialized */
	zassert_true(fdtable_fd_is_initialized(0));
	zassert_true(fdtable_fd_is_initialized(1));
	zassert_true(fdtable_fd_is_initialized(2));
}

ZTEST_SUITE(posix_apis, NULL, NULL, NULL, NULL, NULL);
