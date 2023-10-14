/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_NEWLIB_LIBC
#define _POSIX_C_SOURCE 200809
#endif

#include <errno.h>
#include <string.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static int _strrncmp(const char *a, const char *b, size_t n)
{
	size_t la = strlen(a);
	size_t lb = strlen(b);

	a += la - MIN(la, n);
	b += lb - MIN(lb, n);

	return strncmp(a, b, n);
}

ZTEST(test_c_lib, test_strerror_invalid)
{
/*
 * For reference, with the "C" locale
 * Linux: Unknown error -42
 * macOS: Unknown error: -42
 */
#define UNKNOWN_ERROR_PREFIX    "Unknown error"
#define UNKNOWN_ERROR_SUFFIX(e) " " STRINGIFY(e)

	/* int values well outside of the usual errno range */
	const int error[] = {
		-2147483648,
		-42,
		4242,
		2147483647,
	};
	const char *const shape[] = {
		UNKNOWN_ERROR_PREFIX UNKNOWN_ERROR_SUFFIX(-2147483648),
		UNKNOWN_ERROR_PREFIX UNKNOWN_ERROR_SUFFIX(-42),
		UNKNOWN_ERROR_PREFIX UNKNOWN_ERROR_SUFFIX(4242),
		UNKNOWN_ERROR_PREFIX UNKNOWN_ERROR_SUFFIX(2147483647),
	};
	const char *const suffix[] = {
		UNKNOWN_ERROR_SUFFIX(-2147483648),
		UNKNOWN_ERROR_SUFFIX(-42),
		UNKNOWN_ERROR_SUFFIX(4242),
		UNKNOWN_ERROR_SUFFIX(2147483647),
	};

	BUILD_ASSERT(ARRAY_SIZE(error) == ARRAY_SIZE(shape));
	BUILD_ASSERT(ARRAY_SIZE(shape) == ARRAY_SIZE(suffix));

	for (size_t i = 0; i < ARRAY_SIZE(shape); ++i) {

		TC_PRINT("Checking strerror(%d)\n", error[i]);

		/* consistent behaviour w.r.t. errno with invalid input */
		errno = 0;

		const char *exp = shape[i];
		const char *act = strerror(error[i]);

		/* do not change errno on failure (for consistence) */
		zassert_equal(0, errno);

		/* validate minimal length, e.g. "Unknown error -42", "Unknown error: -42", .. */
		zassert_true(strlen(act) >= strlen(exp), "mismatch: exp: ~'%s' act: '%s'", exp,
			     act);

		/* validate prefix, e.g. "Unknown error" */
		zassert_equal(0,
			      strncmp(UNKNOWN_ERROR_PREFIX, act, sizeof(UNKNOWN_ERROR_PREFIX) - 1));

		/* validate suffix, e.g. " -42" */
		zassert_equal(0, _strrncmp(suffix[i], act, strlen(suffix[i])));
	}
}

ZTEST(test_c_lib, test_strerror)
{
	const char *expected;
	const char *actual;

	errno = 4242;
	expected = IS_ENABLED(CONFIG_MINIMAL_LIBC_DISABLE_STRING_ERROR_TABLE) ? ""
									      : "Invalid argument";
	actual = strerror(EINVAL);
	zassert_equal(0, strcmp(expected, actual),
		      "mismatch: exp: %s act: %s", expected, actual);

	/* do not change errno on success */
	zassert_equal(4242, errno);

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
		zassert_equal(0, strerror_r(EINVAL, actual, n));
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

		zassert_equal(ERANGE, strerror_r(EINVAL, actual, 0));
	}

	/* do not change errno on success */
	zassert_equal(4242, errno);

	errno = 0;
	zassert_equal(EINVAL, strerror_r(-42, actual, n));
	zassert_equal(EINVAL, strerror_r(4242, actual, n));
	/* do not change errno on failure */
	zassert_equal(0, errno);
}
