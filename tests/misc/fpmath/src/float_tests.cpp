/* Copyright (c) 2022 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/math/fp.h>
#include <zephyr/ztest.h>

ZTEST(fpmath, test_printing)
{
	const fp_t value = FLOAT_TO_FP(1.23456789f);
	char expected_string[128] = {0};
	char buffer[128] = {0};
	int expected_num_chars_written = sprintf(
		expected_string, "%." STRINGIFY(CONFIG_MATH_UTIL_PRIf_PRECISION) "f", value);
	int num_chars_written = sprintf(buffer, "%" PRIf, PRIf_ARG(value));

	zassert_equal(expected_num_chars_written, num_chars_written,
		      "Expected to write %d characters, but wrote %d", expected_num_chars_written,
		      num_chars_written);
	zassert_equal(2 + CONFIG_MATH_UTIL_PRIf_PRECISION, num_chars_written,
		      "Expected to write %d characters, but wrote %d",
		      2 + CONFIG_MATH_UTIL_PRIf_PRECISION, num_chars_written);
	zassert_true(strcmp(expected_string, buffer) == 0,
		     "Expected string to be '%s', but was '%s'", expected_string, buffer);
}
