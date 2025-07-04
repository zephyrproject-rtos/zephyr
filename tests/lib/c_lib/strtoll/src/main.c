/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <zephyr/ztest.h>

ZTEST(strtoll_suite, test_llong_max)
{
    char *end;
    errno = 0;
    zassert_equal(LLONG_MAX, strtoll("9223372036854775807", &end, 10), "LLONG_MAX failed");
    zassert_equal(0, errno, "errno should be 0");
}

ZTEST(strtoll_suite, test_llong_min)
{
    char *end;
    errno = 0;
    zassert_equal(LLONG_MIN, strtoll("-9223372036854775808", &end, 10), "LLONG_MIN failed");
    zassert_equal(0, errno, "errno should be 0");
}

ZTEST(strtoll_suite, test_overflow)
{
    char *end;
    errno = 0;
    zassert_equal(LLONG_MAX, strtoll("9223372036854775808", &end, 10), "Overflow failed");
    zassert_equal(ERANGE, errno, "errno should be ERANGE");
}

ZTEST(strtoll_suite, test_underflow)
{
    char *end;
    errno = 0;
    zassert_equal(LLONG_MIN, strtoll("-9223372036854775809", &end, 10), "Underflow failed");
    zassert_equal(ERANGE, errno, "errno should be ERANGE");
}

ZTEST_SUITE(strtoll_suite, NULL, NULL, NULL, NULL, NULL);