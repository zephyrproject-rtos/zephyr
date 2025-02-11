/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "thrd.h"

#include <stdint.h>
#include <threads.h>

#include <zephyr/ztest.h>

static tss_t key;
static int32_t destroyed_values[2];
static const int32_t forty_two = FORTY_TWO;
static const int32_t seventy_three = SEVENTY_THREE;

static thrd_t thread1;
static thrd_t thread2;

static void destroy_fn(void *arg)
{
	int32_t val = *(int32_t *)arg;

	switch (val) {
	case FORTY_TWO:
		destroyed_values[0] = FORTY_TWO;
		break;
	case SEVENTY_THREE:
		destroyed_values[1] = SEVENTY_THREE;
		break;
	default:
		zassert_true(val == FORTY_TWO || val == SEVENTY_THREE, "unexpected val %d", val);
	}
}

ZTEST(libc_tss, test_tss_create_delete)
{
	/* degenerate test cases */
	if (false) {
		/* pthread_key_create() has not been hardened against this */
		zassert_equal(thrd_error, tss_create(NULL, NULL));
		zassert_equal(thrd_error, tss_create(NULL, destroy_fn));
	}
	tss_delete(BIOS_FOOD);

	/* happy path tested in before() / after() */
}

static int thread_fn(void *arg)
{
	int32_t val = *(int32_t *)arg;

	zassert_equal(tss_get(key), NULL);
	tss_set(key, arg);
	zassert_equal(tss_get(key), arg);

	return val;
}

/* test out separate threads doing tss_get() / tss_set() */
ZTEST(libc_tss, test_tss_get_set)
{
	int res1 = BIOS_FOOD;
	int res2 = BIOS_FOOD;

	/* degenerate test cases */
	zassert_is_null(tss_get(BIOS_FOOD));
	zassert_not_equal(thrd_success, tss_set(FORTY_TWO, (void *)BIOS_FOOD));
	zassert_is_null(tss_get(FORTY_TWO));

	zassert_equal(thrd_success, thrd_create(&thread1, thread_fn, (void *)&forty_two));
	zassert_equal(thrd_success, thrd_create(&thread2, thread_fn, (void *)&seventy_three));

	zassert_equal(thrd_success, thrd_join(thread1, &res1));
	zassert_equal(thrd_success, thrd_join(thread2, &res2));
	zassert_equal(FORTY_TWO, res1);
	zassert_equal(SEVENTY_THREE, res2);

	zassert_equal(destroyed_values[0], FORTY_TWO);
	zassert_equal(destroyed_values[1], SEVENTY_THREE);
}

static void before(void *arg)
{
	destroyed_values[0] = 0;
	destroyed_values[1] = 0;

	zassert_equal(thrd_success, tss_create(&key, destroy_fn));
}

static void after(void *arg)
{
	tss_delete(key);
}

ZTEST_SUITE(libc_tss, NULL, NULL, before, after, NULL);
