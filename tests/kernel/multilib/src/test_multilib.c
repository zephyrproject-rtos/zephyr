/*
 * Copyright (c) 2016 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>
#include <tc_util.h>

void main(void)
{
	volatile long long a = 100;
	volatile long long b = 3;
	volatile long long c = a / b;
	int rv = TC_PASS;

	if (c != 33) {
		rv = TC_FAIL;
	}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
