/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "thrd.h"

#include <stdint.h>
#include <threads.h>

#include <zephyr/ztest.h>

static size_t number_of_calls;
static once_flag flag = ONCE_FLAG_INIT;

static void once_func(void)
{
	number_of_calls++;
}

ZTEST(libc_once, test_call_once)
{
	zassert_equal(number_of_calls, 0);

	call_once(&flag, once_func);
	call_once(&flag, once_func);
	call_once(&flag, once_func);

	zassert_equal(number_of_calls, 1);
}

ZTEST_SUITE(libc_once, NULL, NULL, NULL, NULL, NULL);
