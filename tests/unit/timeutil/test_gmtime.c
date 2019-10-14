/*
 * Copyright (c) 2019 Peter Bigot Consulting
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests that gmtime matches gmtime_r */

#include <string.h>
#include <ztest.h>
#include "timeutil_test.h"

void test_gmtime(void)
{
	struct tm tm = {
		/* Initialize an unset field */
		.tm_isdst = 1234,
	};
	time_t time = 1561994005;

	zassert_equal(&tm, gmtime_r(&time, &tm),
		      "gmtime_r return failed");

	struct tm *tp = gmtime(&time);

	zassert_true(memcmp(&tm, tp, sizeof(tm)) == 0,
		     "gmtime disagrees with gmtime_r");
}
