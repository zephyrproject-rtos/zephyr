/* Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dsp/print_format.h>
#include <zephyr/ztest.h>
#include <string.h>

#define float_multiplier(type) ((INT64_C(1) << (8 * sizeof(type) - 1)) - 1)

#define assert_strings(expected, actual)                                                           \
	zexpect_equal(strlen(expected), strlen(actual), "Expected %zu(%s), got %zu(%s)",           \
		      strlen(expected), expected, strlen(actual), actual);                         \
	zexpect_mem_equal(expected, actual, MIN(strlen(expected), strlen(actual)),                 \
			  "Expected '%s', got '%s'", expected, actual)

ZTEST_SUITE(zdsp_print_format, NULL, NULL, NULL, NULL, NULL);

ZTEST(zdsp_print_format, test_print_q31_precision_positive)
{
	char buffer[256];
	q31_t q = (q31_t)0x0f5c28f0; /* 0.119999997 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 0));
	assert_strings("0.119999", buffer);

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 1));
	assert_strings("0.239999", buffer);

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, -2));
	assert_strings("0.029999", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, 0));
	assert_strings("0.1199", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, 1));
	assert_strings("0.2399", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, -2));
	assert_strings("0.0299", buffer);
}

ZTEST(zdsp_print_format, test_print_q31_precision_negative)
{
	char buffer[256];
	q31_t q = (q31_t)0x83d70a00; /* -0.970000029 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 0));
	assert_strings("-0.970000", buffer);

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 1));
	assert_strings("-1.940000", buffer);

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, -2));
	assert_strings("-0.242500", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, 0));
	assert_strings("-0.9700", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, 1));
	assert_strings("-1.9400", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, -2));
	assert_strings("-0.2425", buffer);
}

ZTEST(zdsp_print_format, test_print_q31_precision_1)
{
	char buffer[256];
	q31_t q = (q31_t)0x0f5c28f0; /* 0.119999997 */

	snprintf(buffer, 256, "%" PRIq(1), PRIq_arg(q, 1, 0));
	assert_strings("0.1", buffer);

	snprintf(buffer, 256, "%" PRIq(1), PRIq_arg(q, 1, 5));
	assert_strings("3.8", buffer);

	q = (q31_t)0x83d70a00; /* -0.970000029 */

	snprintf(buffer, 256, "%" PRIq(1), PRIq_arg(q, 1, 0));
	assert_strings("-0.9", buffer);
}

ZTEST(zdsp_print_format, test_print_q31_precision_9)
{
	char buffer[256];
	q31_t q = (q31_t)0x0f5c28f0; /* 0.119999997318 */

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, 0));
	assert_strings("0.119999997", buffer);

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, 1));
	assert_strings("0.239999994", buffer);

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, -2));
	assert_strings("0.029999999", buffer);

	q = (q31_t)0x83d70a00; /* -0.970000028610 */

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, 0));
	assert_strings("-0.970000028", buffer);

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, 1));
	assert_strings("-1.940000057", buffer);

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, -2));
	assert_strings("-0.242500007", buffer);
}

ZTEST(zdsp_print_format, test_print_q31_zero)
{
	char buffer[256];
	q31_t q = 0;

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 0));
	assert_strings("0.000000", buffer);

	/* The sign is kept when a negative value truncates to zero */
	q = -1; /* -0.000000000466 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 0));
	assert_strings("-0.000000", buffer);
}

ZTEST(zdsp_print_format, test_print_q31_large_shift)
{
	char buffer[256];
	q31_t q = INT32_MAX; /* 0.999999999534 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 16));
	assert_strings("65535.999969", buffer);

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 30));
	assert_strings("1073741823.500000", buffer);

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 31));
	assert_strings("2147483647.000000", buffer);

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, -31));
	assert_strings("0.000000", buffer);

	q = -INT32_MAX; /* -0.999999999534 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 16));
	assert_strings("-65535.999969", buffer);

	q = INT32_MIN; /* -1.0 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 16));
	assert_strings("-65536.000000", buffer);

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 31));
	assert_strings("-2147483648.000000", buffer);

	q = (q31_t)0x0f5c28f0; /* 0.119999997 */

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, -20));
	assert_strings("0.000000114", buffer);
}

ZTEST(zdsp_print_format, test_print_q31_full_scale)
{
	char buffer[256];
	q31_t q = INT32_MAX; /* 0.999999999534 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 0));
	assert_strings("0.999999", buffer);

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, 0));
	assert_strings("0.999999999", buffer);

	q = (q31_t)0x40000002; /* 0.500000000931 */

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q, 9, 0));
	assert_strings("0.500000000", buffer);

	q = INT32_MIN; /* -1.0 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q, 6, 0));
	assert_strings("-1.000000", buffer);
}

ZTEST(zdsp_print_format, test_print_q15_q7_full_scale)
{
	char buffer[256];
	q15_t q15 = INT16_MAX; /* 0.999969482 */
	q7_t q7 = INT8_MAX;    /* 0.9921875 */

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q15, 9, 0));
	assert_strings("0.999969482", buffer);

	q15 = INT16_MIN; /* -1.0 */

	snprintf(buffer, 256, "%" PRIq(6), PRIq_arg(q15, 6, 0));
	assert_strings("-1.000000", buffer);

	snprintf(buffer, 256, "%" PRIq(9), PRIq_arg(q7, 9, 0));
	assert_strings("0.992187500", buffer);

	q7 = INT8_MIN; /* -1.0 */

	snprintf(buffer, 256, "%" PRIq(3), PRIq_arg(q7, 3, 0));
	assert_strings("-1.000", buffer);
}

ZTEST(zdsp_print_format, test_print_q15_precision_positive)
{
	char buffer[256];
	q15_t q = (q15_t)0x28f5; /* 0.319986672 */

	snprintf(buffer, 256, "%" PRIq(5), PRIq_arg(q, 5, 0));
	assert_strings("0.31997", buffer);

	snprintf(buffer, 256, "%" PRIq(5), PRIq_arg(q, 5, 1));
	assert_strings("0.63995", buffer);

	snprintf(buffer, 256, "%" PRIq(5), PRIq_arg(q, 5, -2));
	assert_strings("0.07999", buffer);

	snprintf(buffer, 256, "%" PRIq(3), PRIq_arg(q, 3, 0));
	assert_strings("0.319", buffer);

	snprintf(buffer, 256, "%" PRIq(3), PRIq_arg(q, 3, 1));
	assert_strings("0.639", buffer);

	snprintf(buffer, 256, "%" PRIq(3), PRIq_arg(q, 3, -2));
	assert_strings("0.079", buffer);
}

ZTEST(zdsp_print_format, test_print_q15_precision_negative)
{
	char buffer[256];
	q15_t q = (q15_t)0xc4bd; /* -0.462965789 */

	snprintf(buffer, 256, "%" PRIq(5), PRIq_arg(q, 5, 0));
	assert_strings("-0.46298", buffer);

	snprintf(buffer, 256, "%" PRIq(5), PRIq_arg(q, 5, 1));
	assert_strings("-0.92596", buffer);

	snprintf(buffer, 256, "%" PRIq(5), PRIq_arg(q, 5, -2));
	assert_strings("-0.11574", buffer);

	snprintf(buffer, 256, "%" PRIq(3), PRIq_arg(q, 3, 0));
	assert_strings("-0.462", buffer);

	snprintf(buffer, 256, "%" PRIq(3), PRIq_arg(q, 3, 1));
	assert_strings("-0.925", buffer);

	snprintf(buffer, 256, "%" PRIq(3), PRIq_arg(q, 3, -2));
	assert_strings("-0.115", buffer);
}

ZTEST(zdsp_print_format, test_print_q7_precision_positive)
{
	char buffer[256];
	q7_t q = (q7_t)0x01; /* 0.007874016 */

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, 0));
	assert_strings("0.0078", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, 1));
	assert_strings("0.0156", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, -2));
	assert_strings("0.0019", buffer);

	snprintf(buffer, 256, "%" PRIq(2), PRIq_arg(q, 2, 0));
	assert_strings("0.00", buffer);

	snprintf(buffer, 256, "%" PRIq(2), PRIq_arg(q, 2, 1));
	assert_strings("0.01", buffer);

	snprintf(buffer, 256, "%" PRIq(2), PRIq_arg(q, 2, -2));
	assert_strings("0.00", buffer);
}

ZTEST(zdsp_print_format, test_print_q7_precision_negative)
{
	char buffer[256];
	q7_t q = (q7_t)0xe2; /* -0.228346457 */

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, 0));
	assert_strings("-0.2343", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, 1));
	assert_strings("-0.4687", buffer);

	snprintf(buffer, 256, "%" PRIq(4), PRIq_arg(q, 4, -2));
	assert_strings("-0.0585", buffer);

	snprintf(buffer, 256, "%" PRIq(2), PRIq_arg(q, 2, 0));
	assert_strings("-0.23", buffer);

	snprintf(buffer, 256, "%" PRIq(2), PRIq_arg(q, 2, 1));
	assert_strings("-0.46", buffer);

	snprintf(buffer, 256, "%" PRIq(2), PRIq_arg(q, 2, -2));
	assert_strings("-0.05", buffer);
}
