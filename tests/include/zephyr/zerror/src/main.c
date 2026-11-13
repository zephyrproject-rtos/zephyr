/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright The Zephyr Project Contributors
 */

#include <zephyr/ztest.h>
#include <zephyr/zerror/zerror.h>

#define TEST_MODULE      (0x42)
#define TEST_CUSTOM_CODE (0x12)
#define TEST_ERRNO       (EINVAL)

ZTEST(test_zerror, test_zero_when_all_fields_are_zero)
{
	zerror_t err = ZERROR(0, 0, 0);

	zassert_equal(err, 0, "ZERROR with all-zero fields should be 0");
}

ZTEST(test_zerror, test_get_module_returns_packed_module)
{
	zerror_t err = ZERROR(TEST_MODULE, TEST_CUSTOM_CODE, TEST_ERRNO);

	zassert_equal(ZERROR_GET_MODULE(err), TEST_MODULE, "Module should be extracted unchanged");
}

ZTEST(test_zerror, test_get_module_returns_packed_module_on_negative_zerror)
{
	zerror_t err = -ZERROR(TEST_MODULE, TEST_CUSTOM_CODE, TEST_ERRNO);

	zassert_equal(ZERROR_GET_MODULE(err), TEST_MODULE,
		      "Module should be extracted unchanged on negative zerror");
}

ZTEST(test_zerror, test_get_custom_code_returns_packed_custom_code)
{
	zerror_t err = ZERROR(TEST_MODULE, TEST_CUSTOM_CODE, TEST_ERRNO);

	zassert_equal(ZERROR_GET_CUSTOM_CODE(err), TEST_CUSTOM_CODE,
		      "Custom code should be extracted unchanged");
}

ZTEST(test_zerror, test_get_custom_code_returns_packed_custom_code_on_negative_zerror)
{
	zerror_t err = -ZERROR(TEST_MODULE, TEST_CUSTOM_CODE, TEST_ERRNO);

	zassert_equal(ZERROR_GET_CUSTOM_CODE(err), TEST_CUSTOM_CODE,
		      "Custom code should be extracted unchanged on negative zerror");
}

ZTEST(test_zerror, test_get_errno_returns_packed_errno)
{
	zerror_t err = ZERROR(TEST_MODULE, TEST_CUSTOM_CODE, TEST_ERRNO);

	zassert_equal(ZERROR_GET_ERRNO(err), TEST_ERRNO, "Errno should be extracted unchanged");
}

ZTEST(test_zerror, test_get_errno_returns_packed_errno_on_negative_zerror)
{
	zerror_t err = -ZERROR(TEST_MODULE, TEST_CUSTOM_CODE, TEST_ERRNO);

	zassert_equal(ZERROR_GET_ERRNO(err), TEST_ERRNO,
		      "Errno should be extracted unchanged on negative zerror %0x, %d", err,
		      ZERROR_GET_ERRNO(err));
}

ZTEST(test_zerror, test_fields_are_independent)
{
	zerror_t err = ZERROR(TEST_MODULE, TEST_CUSTOM_CODE, TEST_ERRNO);

	zassert_equal(ZERROR_GET_MODULE(err), TEST_MODULE,
		      "Module should not be affected by other fields");
	zassert_equal(ZERROR_GET_CUSTOM_CODE(err), TEST_CUSTOM_CODE,
		      "Custom code should not be affected by other fields");
	zassert_equal(ZERROR_GET_ERRNO(err), TEST_ERRNO,
		      "Errno should not be affected by other fields");
}

ZTEST(test_zerror, test_module_max_value)
{
	/* The module field is limited to 7 bits. The maximum value is 0x7F. */
	zerror_t err = ZERROR(0x7F, 0, 0);

	zassert_equal(ZERROR_GET_MODULE(err), 0x7F, "Maximum 7-bit module should be preserved");
}

ZTEST(test_zerror, test_module_overflow_is_masked)
{
	/* Bits above the 7-bit module field must be masked off. */
	zerror_t err = ZERROR(0xFF, 0, 0);

	zassert_equal(ZERROR_GET_MODULE(err), 0x7F, "Module larger than 7 bits should be masked");
}

ZTEST(test_zerror, test_negative_custom_code_is_sign_extended)
{
	zerror_t err = ZERROR(TEST_MODULE, -TEST_CUSTOM_CODE, TEST_ERRNO);

	zassert_equal(ZERROR_GET_CUSTOM_CODE(err), -TEST_CUSTOM_CODE,
		      "Negative custom code should be sign extended");
}

ZTEST(test_zerror, test_negative_errno_is_sign_extended)
{
	zerror_t err = ZERROR(TEST_MODULE, TEST_CUSTOM_CODE, -EINVAL);

	zassert_equal(ZERROR_GET_ERRNO(err), -EINVAL, "Negative errno should be sign extended");
}

ZTEST(test_zerror, test_max_errno_value)
{
	zerror_t err = ZERROR(0, 0, INT16_MAX);

	zassert_equal(ZERROR_GET_ERRNO(err), INT16_MAX, "Maximum 16-bit errno should be preserved");
}

zerror_t return_on_error_from_errno_expression(void)
{
	/* The expression passed to the macro returns an errno instead of a zerror_t.
	 * This should be avoided because the error code, when extracted from the zerror_r, will
	 * lose the sign.
	 */
	ZERROR_RETURN_ON_ERROR(-TEST_ERRNO);
	return 0;
}

ZTEST(test_zerror, test_int_to_zerror_conversion)
{
	zerror_t err = return_on_error_from_errno_expression();

	zassert_true(err < 0, "Zerror from errno should be less than zero");
	zassert_equal(ZERROR_GET_MODULE(err), 0,
		      "Zerror from errno should have zero module, %0x, %d", err,
		      ZERROR_GET_MODULE(err));
	zassert_equal(ZERROR_GET_CUSTOM_CODE(err), 0,
		      "Zerror from errno should have zero custom code, %0x, %d", err,
		      ZERROR_GET_CUSTOM_CODE(err));

	/* Since the errno sign is not retained in this case, we expect a positive number */
	zassert_equal(ZERROR_GET_ERRNO(err), TEST_ERRNO,
		      "Zerror from errno should have correct errno! %0x, %d", err,
		      ZERROR_GET_ERRNO(err));
}

ZTEST_SUITE(test_zerror, NULL, NULL, NULL, NULL, NULL);
