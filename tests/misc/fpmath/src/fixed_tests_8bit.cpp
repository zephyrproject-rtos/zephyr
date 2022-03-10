/* Copyright (c) 2022 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/math/fp.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <stdio.h>

ZTEST(fpmath, test_convert_positive_float_to_fp)
{
	const fp_t value = FLOAT_TO_FP(1193046.46875f);
	const int32_t int_val = INT32_C(0x12345680);

	zassert_equal(value, int_val, "Expected 0x%x, but got 0x%x", int_val, value);
}

ZTEST(fpmath, test_convert_negative_float_to_fp)
{
	const fp_t value = FLOAT_TO_FP(-904106.5f);
	const int32_t int_val = INT32_C(0xf2345580);

	zassert_equal(value, int_val, "Expected 0x%x, but got 0x%x", int_val, value);
}

ZTEST(fpmath, test_printing_positive_value)
{
	const char *expect_buffer = "1193046.500000";
	const fp_t value = FLOAT_TO_FP(1193046.46875f);
	char buffer[128] = {0};
	sprintf(buffer, "%" PRIf, PRIf_ARG(value));

	zassert_ok(strcmp(expect_buffer, buffer), "Expected '%s', but got '%s'", expect_buffer, buffer);
}

ZTEST(fpmath, test_printing_negative_value)
{
	const char *expect_buffer = "-904106.500000";
	const fp_t value = FLOAT_TO_FP(-904106.46875f);
	char buffer[128] = {0};
	sprintf(buffer, "%" PRIf, PRIf_ARG(value));

	zassert_ok(strcmp(expect_buffer, buffer), "Expected '%s', but got '%s'", expect_buffer, buffer);
}