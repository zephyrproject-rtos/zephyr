/*
 * Copyright (c) 2019 Peter Bigot Consulting
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CIVILTIME_TEST_H
#define CIVILTIME_TEST_H

#include <stdlib.h>
#include <misc/civiltime.h>

struct civiltime_test_data {
	time_t unix;
	const char *civil;
	struct tm tm;
};

void civiltime_check(const struct civiltime_test_data *tp,
		     size_t count);

void test_s32(void);
void test_s64(void);

#endif /* CIVILTIME_TEST_H */
