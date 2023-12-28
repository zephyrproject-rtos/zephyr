/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

ZTEST(libc_strerror, test_strerror)
{
	const char *expected;
	const char *actual;

	errno = 4242;
	if (IS_ENABLED(CONFIG_MINIMAL_LIBC_DISABLE_STRING_ERROR_TABLE)) {
		expected = "";
		actual = strerror(EINVAL);
	} else {
		expected = "Invalid argument";
		actual = strerror(EINVAL);
	}
	zassert_equal(0, strcmp(expected, actual),
		      "mismatch: exp: %s act: %s", expected, actual);

	/* do not change errno on success */
	zassert_equal(4242, errno, "");

#ifndef CONFIG_EXTERNAL_LIBC
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
#endif

	/* consistent behaviour for "Success" */
	if (!IS_ENABLED(CONFIG_MINIMAL_LIBC_DISABLE_STRING_ERROR_TABLE)) {
		expected = "Success";
		actual = strerror(0);
		zassert_equal(0, strcmp(expected, actual),
			      "mismatch: exp: %s act: %s", expected, actual);
	}
}

ZTEST_SUITE(libc_strerror, NULL, NULL, NULL, NULL, NULL);
