/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <threads.h>

#include <zephyr/sys_clock.h>
#include <zephyr/ztest.h>

ZTEST(test_c_lib, test_thrd_sleep)
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
		*x = 0xb105f00d;
	}

	return 42;
}

ZTEST(test_c_lib, test_thrd_create_join)
{
	thrd_t thr;
	int res = 0;
	uintptr_t x = 0;
	thrd_start_t fun = thrd_create_join_fn;

	if (false) {
		/* pthread_create() is not hardened for degenerate cases like this */
		zassert_equal(thrd_error, thrd_create(NULL, NULL, NULL));
		zassert_equal(thrd_error, thrd_create(NULL, NULL, &x));
		zassert_equal(thrd_error, thrd_create(NULL, fun, NULL));
		zassert_equal(thrd_error, thrd_create(NULL, fun, &x));
		zassert_equal(thrd_error, thrd_create(&thr, NULL, NULL));
		zassert_equal(thrd_error, thrd_create(&thr, NULL, &x));
	}

	zassert_equal(thrd_success, thrd_create(&thr, fun, NULL));
	zassert_equal(thrd_success, thrd_join(thr, NULL));

	zassert_equal(thrd_success, thrd_create(&thr, fun, &x));
	zassert_equal(thrd_success, thrd_join(thr, &res));
	zassert_equal(0xb105f00d, x, "expected: %d actual: %d", 0xb105f00d, x);
	zassert_equal(42, res);
}

static int thrd_exit_fn(void *arg)
{
	uintptr_t *x = (uintptr_t *)arg;

	*x = 0xb105f00d;

	thrd_exit(73);

	return 42;

	CODE_UNREACHABLE;
}

ZTEST(test_c_lib, test_thrd_exit)
{
	thrd_t thr;
	int res = 0;
	uintptr_t x = 0;

	zassert_equal(thrd_success, thrd_create(&thr, thrd_exit_fn, &x));
	zassert_equal(thrd_success, thrd_join(thr, &res));
	zassert_equal(0xb105f00d, x);
	zassert_equal(73, res);
}

ZTEST(test_c_lib, test_thrd_yield)
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

ZTEST(test_c_lib, test_thrd_current_equal)
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
	return 73;
}

ZTEST(test_c_lib, test_thrd_detach)
{
	thrd_t thr;

	zassert_equal(thrd_success, thrd_create(&thr, thrd_detach_fn, NULL));
	zassert_equal(thrd_success, thrd_detach(thr));
	zassert_equal(thrd_error, thrd_join(thr, NULL));

	do {
		k_msleep(100);
	} while (!detached_thread_is_probably_done);
}

ZTEST(test_c_lib, test_thrd_reuse)
{
	thrd_t thr;

	for (int i = 0; i < 42; ++i) {
		zassert_equal(thrd_success, thrd_create(&thr, thrd_create_join_fn, NULL));
		zassert_equal(thrd_success, thrd_join(thr, NULL));
	}
}
