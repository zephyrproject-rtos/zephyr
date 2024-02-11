/*
 * Copyright (c) 2024 Dawid Osuchowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <stdio.h>
#include <errno.h>

ZTEST(stdio, test_fgetc)
{
    int fd = -1;
    int ret = fgetc(stdin);

    zassert_equal(ret, -1, "Expected return value -1, got %d", ret);
    zassert_equal(errno, ENOSYS, "Expected errno ENOSYS, got %d", errno);

}

ZTEST_SUITE(stdio, NULL, NULL, NULL, NULL, NULL);