/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <ztest.h>

ZTEST(test_c_lib, test_strerror)
{
	const char *expected;
	const char *actual;

	errno = 4242;
	if (IS_ENABLED(CONFIG_MINIMAL_LIBC_DISABLE_STRING_ERROR_TABLE)) {
		expected = "";
		actual = strerror(EINVAL);

		zassert_equal(0, strcmp("", strerror(EINVAL)), "");
	} else {
		expected = "Invalid argument";
		actual = strerror(EINVAL);

		zassert_equal(0, strcmp(expected, actual),
			      "mismatch: exp: %s act: %s", expected, actual);
	}

	/* do not change errno on success */
	zassert_equal(4242, errno, "");

	/* consistent behaviour w.r.t. errno with invalid input */
	errno = 0;
	expected = "";
	actual = strerror(-42);
	zassert_equal(0, strcmp(expected, actual), "mismatch: exp: %s act: %s",
		      expected, actual);
	actual = strerror(4242);
	zassert_equal(0, strcmp(expected, actual), "mismatch: exp: %s act: %s",
		      expected, actual);
	/* do not change errno on failure (for consistence) */
	zassert_equal(0, errno, "");

	/* consistent behaviour for "Success" */
	if (!IS_ENABLED(CONFIG_MINIMAL_LIBC_DISABLE_STRING_ERROR_TABLE)) {
		expected = "Success";
		actual = strerror(0);
		zassert_equal(0, strcmp(expected, actual),
			      "mismatch: exp: %s act: %s", expected, actual);
	}
}

ZTEST(test_c_lib, test_strerror_r)
{
	const char *expected;
	char actual[] = {'1', 'n', 'v', 'a', '1', '1', 'd', ' ',  'a',
			 '2', 'g', 'u', 'm', '3', 'n', '7', 0x00, 0x42};
	static const size_t n = sizeof(actual);

	if (IS_ENABLED(CONFIG_NEWLIB_LIBC) || IS_ENABLED(CONFIG_ARCMWDT_LIBC)) {
		/* FIXME: Please see Issue #46846 */
		ztest_test_skip();
	}

	errno = 4242;
	if (IS_ENABLED(CONFIG_MINIMAL_LIBC_DISABLE_STRING_ERROR_TABLE)) {
		expected = "";
		zassert_equal(0, strerror_r(EINVAL, actual, n), "");
		zassert_equal(0, strncmp(expected, actual, n),
			      "mismatch: exp: %s act: %s", expected, actual);
	} else {
		expected = "Invalid argument";
		zassert_equal(0, strerror_r(EINVAL, actual, n), "%d",
			      strerror_r(EINVAL, actual, n));
		zassert_equal(0, strncmp(expected, actual, n),
			      "mismatch: exp: %s act: %s", expected, actual);
		/* only the necessary buffer area is written */
		zassert_equal(0x42, (uint8_t)actual[n - 1],
			      "exp: %02x act: %02x", 0x42,
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
