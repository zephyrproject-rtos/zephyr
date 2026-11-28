/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "thrd.h"

#include <stdint.h>
#include <threads.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>
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

ZTEST(libc_tss, test_tss_create_exhaustion)
{
	tss_t keys[CONFIG_POSIX_THREAD_KEYS_MAX];
	size_t count;
	int ret = thrd_success;

	/* before() already holds one key, so this must exhaust the key pool. */
	for (count = 0; count < ARRAY_SIZE(keys); ++count) {
		ret = tss_create(&keys[count], NULL);
		if (ret != thrd_success) {
			break;
		}
	}

	for (size_t i = 0; i < count; ++i) {
		tss_delete(keys[i]);
	}

	zassert_equal(thrd_error, ret, "unexpected status %d", ret);
}

static int tss_set_with_exhausted_heap(void *arg)
{
	sys_slist_t allocations = SYS_SLIST_STATIC_INIT(&allocations);
	sys_snode_t *node;
	int ret;

	for (node = k_malloc(sizeof(*node)); node != NULL; node = k_malloc(sizeof(*node))) {
		sys_slist_prepend(&allocations, node);
	}

	ret = tss_set(key, arg);

	for (node = sys_slist_get(&allocations); node != NULL; node = sys_slist_get(&allocations)) {
		k_free(node);
	}

	zassert_equal(tss_set(key, arg), thrd_success);
	zassert_equal_ptr(tss_get(key), arg);

	return ret;
}

ZTEST(libc_tss, test_tss_set_exhaustion)
{
	int32_t value = FORTY_TWO;
	thrd_t thread;
	int ret;

	zassert_equal(thrd_create(&thread, tss_set_with_exhausted_heap, &value), thrd_success);
	zassert_equal(thrd_join(thread, &ret), thrd_success);
	zassert_equal(ret, thrd_error, "unexpected status %d", ret);
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
