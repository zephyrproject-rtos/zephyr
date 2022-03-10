/* Copyright (c) 2022 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/math/fp.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

ZTEST(fpmath, test_printing_positive_value)
{
	const char *expect_buffer = "4660.337890";
	const fp_t value = FLOAT_TO_FP(4660.337769f);
	char buffer[128] = {0};
	sprintf(buffer, "%" PRIf, PRIf_ARG(value));

	zassert_ok(strcmp(expect_buffer, buffer), "Expected '%s', but got '%s'", expect_buffer,
		   buffer);
}

ZTEST(fpmath, test_printing_negative_value)
{
	const char *expect_buffer = "-3532.337890";
	const fp_t value = FLOAT_TO_FP(-3532.337769f);
	char buffer[128] = {0};
	sprintf(buffer, "%" PRIf, PRIf_ARG(value));

	zassert_ok(strcmp(expect_buffer, buffer), "Expected '%s', but got '%s'", expect_buffer,
		   buffer);
}