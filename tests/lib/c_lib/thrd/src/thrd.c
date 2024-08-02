/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "thrd.h"

#include <stdint.h>
#include <threads.h>

#include <zephyr/sys_clock.h>
#include <zephyr/ztest.h>

static thrd_t     thr;
static uintptr_t  param;

ZTEST(libc_thrd, test_thrd_sleep)
{
	int64_t end;
	int64_t start;
	struct timespec duration = {0};
	struct timespec remaining;
	const uint16_t delay_ms[] = {0, 100, 200, 400};

	zassert_not_equal(0, thrd_sleep(NULL, NULL));
	zassert_ok(thrd_sleep(&duration, NULL));
	zassert_ok(thrd_sleep(&duration, &duration));

	for (int i = 0; i < ARRAY_SIZE(delay_ms); ++i) {
		duration = (struct timespec){.tv_nsec = delay_ms[i] * NSEC_PER_MSEC};
		remaining = (struct timespec){.tv_sec = 4242, .tv_nsec = 4242};

		printk("sleeping %d ms\n", delay_ms[i]);
		start = k_uptime_get();
		zassert_ok(thrd_sleep(&duration, &remaining));
		end = k_uptime_get();
		zassert_equal(remaining.tv_sec, 0);
		zassert_equal(remaining.tv_nsec, 0);
		zassert_true(end - start >= delay_ms[i]);
	}
}

static int thrd_create_join_fn(void *arg)
{
	uintptr_t *x = (uintptr_t *)arg;

	if (x != NULL) {
		*x = BIOS_FOOD;
	}

	return FORTY_TWO;
}

ZTEST(libc_thrd, test_thrd_create_join)
{
	int res = 0;
	thrd_start_t fun = thrd_create_join_fn;

	param = 0;

	if (false) {
		/* pthread_create() is not hardened for degenerate cases like this */
		zassert_equal(thrd_error, thrd_create(NULL, NULL, NULL));
		zassert_equal(thrd_error, thrd_create(NULL, NULL, &param));
		zassert_equal(thrd_error, thrd_create(NULL, fun, NULL));
		zassert_equal(thrd_error, thrd_create(NULL, fun, &param));
		zassert_equal(thrd_error, thrd_create(&thr, NULL, NULL));
		zassert_equal(thrd_error, thrd_create(&thr, NULL, &param));
	}

	zassert_equal(thrd_success, thrd_create(&thr, fun, NULL));
	zassert_equal(thrd_success, thrd_join(thr, NULL));

	zassert_equal(thrd_success, thrd_create(&thr, fun, &param));
	zassert_equal(thrd_success, thrd_join(thr, &res));
	zassert_equal(BIOS_FOOD, param, "expected: %d actual: %d", BIOS_FOOD, param);
	zassert_equal(FORTY_TWO, res);
}

static int thrd_exit_fn(void *arg)
{
	uintptr_t *x = (uintptr_t *)arg;

	*x = BIOS_FOOD;

	thrd_exit(SEVENTY_THREE);

	return FORTY_TWO;

	CODE_UNREACHABLE;
}

ZTEST(libc_thrd, test_thrd_exit)
{
	int res = 0;

	param = 0;

	zassert_equal(thrd_success, thrd_create(&thr, thrd_exit_fn, &param));
	zassert_equal(thrd_success, thrd_join(thr, &res));
	zassert_equal(BIOS_FOOD, param);
	zassert_equal(SEVENTY_THREE, res);
}

ZTEST(libc_thrd, test_thrd_yield)
{
	thrd_yield();
}

static thrd_t child;
static thrd_t parent;

static int thrd_current_equal_fn(void *arg)
{
	ARG_UNUSED(arg);

	zassert_equal(thrd_current(), child);
	zassert_not_equal(child, parent);

	zassert_true(thrd_equal(thrd_current(), child));
	zassert_false(thrd_equal(child, parent));

	return 0;
}

ZTEST(libc_thrd, test_thrd_current_equal)
{
	parent = thrd_current();

	zassert_equal(thrd_success, thrd_create(&child, thrd_current_equal_fn, NULL));
	zassert_equal(thrd_success, thrd_join(child, NULL));
}

static bool detached_thread_is_probably_done;

static int thrd_detach_fn(void *arg)
{
	ARG_UNUSED(arg);

	detached_thread_is_probably_done = true;
	return SEVENTY_THREE;
}

ZTEST(libc_thrd, test_thrd_detach)
{
	zassert_equal(thrd_success, thrd_create(&thr, thrd_detach_fn, NULL));
	zassert_equal(thrd_success, thrd_detach(thr));
	zassert_equal(thrd_error, thrd_join(thr, NULL));

	do {
		k_msleep(100);
	} while (!detached_thread_is_probably_done);

	zassert_equal(thrd_error, thrd_join(thr, NULL));
}

ZTEST(libc_thrd, test_thrd_reuse)
{
	for (int i = 0; i < FORTY_TWO; ++i) {
		zassert_equal(thrd_success, thrd_create(&thr, thrd_create_join_fn, NULL));
		zassert_equal(thrd_success, thrd_join(thr, NULL));
	}
}

ZTEST_SUITE(libc_thrd, NULL, NULL, NULL, NULL, NULL);
