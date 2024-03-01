/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST(k_strnlen, test_k_strnlen)
{
#define BUFSIZE 10

	char buffer[BUFSIZE];

	(void)memset(buffer, '\0', BUFSIZE);
	(void)memset(buffer, 'b', 5); /* 5 is BUFSIZE / 2 */

	zassert_equal(k_strnlen(buffer, BUFSIZE / 3), BUFSIZE / 3, "k_strnlen failed");
	zassert_equal(k_strnlen(buffer, BUFSIZE), BUFSIZE / 2, "k_strnlen failed");
}

ZTEST_SUITE(k_strnlen, NULL, NULL, NULL, NULL, NULL);
