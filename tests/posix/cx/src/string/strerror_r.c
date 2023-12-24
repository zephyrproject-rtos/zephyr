/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

ZTEST(posix_cx_string, test_strerror_r)
{
	const char *expected;
	char actual[] = {'1', 'n', 'v', 'a', '1', '1', 'd', ' ',  'a',
			 '2', 'g', 'u', 'm', '3', 'n', '7', 0x00, 0x42};
	static const size_t n = sizeof(actual);

	errno = 4242;
	if (!IS_ENABLED(CONFIG_COMMON_LIBC_STRING_ERROR_TABLE)) {
		expected = "";
		zassert_equal(0, strerror_r(EINVAL, actual, n), "");
		zassert_equal(0, strncmp(expected, actual, n), "mismatch: exp: %s act: %s",
			      expected, actual);
	} else {
		expected = "Invalid argument";
		zassert_equal(0, strerror_r(EINVAL, actual, n), "%d",
			      strerror_r(EINVAL, actual, n));
		zassert_equal(0, strncmp(expected, actual, n), "mismatch: exp: %s act: %s",
			      expected, actual);
		/* only the necessary buffer area is written */
		zassert_equal(0x42, (uint8_t)actual[n - 1], "exp: %02x act: %02x", 0x42,
			      (uint8_t)actual[n - 1]);

		zassert_equal(ERANGE, strerror_r(EINVAL, actual, 0), "");
	}

	/* do not change errno on success */
	zassert_equal(4242, errno, "");

	errno = 0;
	zassert_equal(EINVAL, strerror_r(-42, actual, n), "");
	zassert_equal(EINVAL, strerror_r(4242, actual, n), "");
	/* do not change errno on failure */
	zassert_equal(0, errno, "");
}
