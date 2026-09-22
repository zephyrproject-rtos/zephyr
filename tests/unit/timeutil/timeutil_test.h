/*
 * Copyright 2019-2020 Peter Bigot Consulting
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TIMEUTIL_TEST_H
#define TIMEUTIL_TEST_H

#include <stdlib.h>
#include <zephyr/sys/timeutil.h>

struct timeutil_test_data {
	time_t ux;
	const char *civil;
	struct tm tm;
};

void timeutil_check(const struct timeutil_test_data *tp,
		    size_t count);

void test_gmtime(void);
void test_s32(void);
void test_s64(void);
void test_sync(void);

#endif /* TIMEUTIL_TEST_H */
