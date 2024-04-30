/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <unistd.h>

ZTEST(getentropy_test_suite, test_getentropy_too_large)
{
	uint8_t buf[256 + 1] = { 0 };
	int ret;

	ret = getentropy(buf, sizeof(buf));
	zassert_equal(ret, -1);
	zassert_equal(errno, EIO);
}

ZTEST(getentropy_test_suite, test_getentropy_null_buffer)
{
	int ret;

	ret = getentropy(NULL, 0);
	zassert_equal(ret, -1);
	zassert_equal(errno, EFAULT);
}

ZTEST(getentropy_test_suite, test_getentropy_max_size)
{
	uint8_t buf[256] = { 0 };
	int ret;

	ret = getentropy(buf, sizeof(buf));
	zassert_equal(ret, 0);
}

ZTEST(getentropy_test_suite, test_getentropy)
{
	uint8_t zero[16] = { 0 };
	uint8_t buf1[16];
	uint8_t buf2[16];
	int ret;

	ret = getentropy(buf1, sizeof(buf1));
	zassert_equal(ret, 0);

	ret = getentropy(buf2, sizeof(buf2));
	zassert_equal(ret, 0);

	zassert_true(memcmp(buf1, zero, sizeof(zero)) != 0);
	zassert_true(memcmp(buf2, zero, sizeof(zero)) != 0);
	zassert_true(memcmp(buf1, buf2, sizeof(buf1)) != 0);
}

ZTEST_SUITE(getentropy_test_suite, NULL, NULL, NULL, NULL, NULL);
