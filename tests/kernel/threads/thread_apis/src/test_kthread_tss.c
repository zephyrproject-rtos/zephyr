/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define N_THR 2
#define N_KEY 2

ZTEST_BMEM static int entry_mode;
ZTEST_BMEM static void *keys[N_KEY];
ZTEST_BMEM static int alloc_count[N_THR];
ZTEST_BMEM static int dtor_count;

static K_SEM_DEFINE(tss_held, 0, 1);
static K_SEM_DEFINE(tss_release, 0, 1);

static void entry(void *arg1, void *arg2, void *arg3);

/* array of thread stacks */
#define K_THREAD_ARRAY_DEFINE_ELEM(idx, name_base, stack_size, entry, prio, options, delay)        \
	K_THREAD_DEFINE(name_base##_##idx, stack_size, entry, NULL, NULL, NULL, prio, options,     \
			delay)

#define K_THREAD_ARRAY_ELEM_ID(idx, name_base) &_k_thread_obj_##name_base##_##idx

#define K_THREAD_ARRAY_STACK_ELEM_ID(idx, name_base)                                               \
	(k_thread_stack_t *)&_k_thread_stack_##name_base##_##idx

#define K_THREAD_ARRAY_DEFINE_STATIC(name, num, stack_size, entry, prio, options, delay)           \
	LISTIFY(num, K_THREAD_ARRAY_DEFINE_ELEM, (;), \
		  name, stack_size, entry, prio, options, delay);      \
	ZTEST_DMEM __maybe_unused static struct k_thread *name[] = {                               \
		LISTIFY(num, K_THREAD_ARRAY_ELEM_ID, (,), name) };                                    \
	ZTEST_DMEM __maybe_unused static k_thread_stack_t *name##_stack[] = {                      \
		LISTIFY(num, K_THREAD_ARRAY_STACK_ELEM_ID, (,), name) }

K_THREAD_ARRAY_DEFINE_STATIC(thread, N_THR, CONFIG_ZTEST_STACK_SIZE, entry, K_PRIO_PREEMPT(10),
			     K_USER, K_TICKS_FOREVER);

static int get_thread_index(void)
{
	for (int i = 0; i < N_THR; i++) {
		if (k_current_get() == thread[i]) {
			return i;
		}
	}

	return -1;
}

static void entry(void *arg1, void *arg2, void *arg3)
{
	void *value;
	void *expected_value;

	if (entry_mode == 0) {
		value = (void *)0x42;
		expected_value = value;
		zassert_not_null(value, "thread could not allocate storage");
		zassert_ok(k_thread_setspecific(keys[0], value), "k_thread_setspecific failed");
		zassert_ok(k_thread_getspecific(keys[0], &value));
		zassert_equal(value, expected_value, "set and retrieved values are different");
	}

	if (entry_mode == 1) {
		value = (void *)0x73;
		expected_value = value;
		for (int i = 0; i < N_KEY; i++) {
			zassert_ok(k_thread_setspecific(keys[i], value));
			zassert_ok(k_thread_getspecific(keys[i], &value));
			zassert_equal(value, expected_value);
		}
	}

	if (entry_mode == 3) {
		int value = 42;
		int thread_index = get_thread_index();
		int *const count = &alloc_count[thread_index];

		while (1) {
			BUILD_ASSERT((N_KEY % N_THR == 0) && (N_KEY / N_THR > 0),
				     "N_KEY must be a multiple of N_THR for this test");
			/*
			 * This loop will keep creating and setting thread-specific keys until the
			 * thread hits the limit on how many keys it can set. At that point, the
			 * thread should have CONFIG_THREAD_SPECIFIC_STORAGE_KEYS_MAX / N_THR keys
			 * set, and thus alloc_count should be equal across all threads, which is
			 * verified in the test function. */
			if (*count == N_KEY / N_THR) {
				break;
			}

			if (k_thread_key_create(&keys[thread_index], NULL) < 0) {
				break;
			}
			if (k_thread_setspecific(keys[thread_index], &value) == -ENOMEM) {
				break;
			};
			*count += 1;
		}
	}

	if (entry_mode == 4) {
		for (int i = 0; i < N_KEY; i++) {
			zassert_ok(k_thread_setspecific(keys[i], INT_TO_POINTER(0x42 + i)));
		}
	}

	if (entry_mode == 5) {
		/* hold a value while the key is deleted by the test thread */
		zassert_ok(k_thread_setspecific(keys[0], (void *)0x42));
		k_sem_give(&tss_held);
		k_sem_take(&tss_release, K_FOREVER);
		zassert_equal(k_thread_setspecific(keys[0], (void *)0x73), -EINVAL);
	}

	if (entry_mode == 6) {
		/* a NULL value has no destructor call; the other value's destructor stores again */
		zassert_ok(k_thread_setspecific(keys[0], NULL));
		zassert_ok(k_thread_setspecific(keys[1], (void *)0x42));
	}
}

static void make_key(void)
{
	zassert_ok(k_thread_key_create(&keys[0], NULL), "insufficient memory to create key");
}

static void make_keys(void)
{
	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(k_thread_key_create(&keys[i], NULL),
			   "insufficient memory to create keys");
	}
}

ZTEST_USER(threads_tss, test_key_popspecific)
{
	void *value = NULL;
	k_thread_key_destructor_t dtor = (k_thread_key_destructor_t)0x73;

	zassert_equal(k_thread_popspecific(&keys[0], &value, &dtor), -ENOENT);
	zassert_equal(k_thread_specific_count(), 0);
	zassert_ok(k_thread_key_create(&keys[0], NULL));
	zassert_ok(k_thread_setspecific(keys[0], (void *)0x42));
	zassert_equal(k_thread_specific_count(), 1);
	zassert_ok(k_thread_popspecific(&keys[1], &value, &dtor));
	zassert_equal(k_thread_specific_count(), 0);
	zassert_equal(keys[0], keys[1]);
	zassert_equal(value, (void *)0x42);
	zassert_equal(dtor, NULL);
	zassert_ok(k_thread_key_delete(keys[0]));
}

ZTEST_USER(threads_tss, test_key_1toN_thread)
{
	make_key();

	/* Different threads set different values to same key */
	entry_mode = 0;
	for (int i = 0; i < N_THR; i++) {
		k_thread_start(thread[i]);
	}

	for (int i = 0; i < N_THR; i++) {
		zassert_ok(k_thread_rejoin(thread[i], NULL, K_FOREVER), "failed to join thread %d",
			   i);
	}

	/* At this point, there should be no more thread-specific values associated with key[0] */
	zassert_ok(k_thread_key_delete(keys[0]), "attempt to delete key failed");
}

ZTEST_USER(threads_tss, test_key_Nto1_thread)
{
	make_keys();

	/* Single thread associates its value with different keys */

	entry_mode = 1;
	k_thread_start(thread[0]);
	zassert_ok(k_thread_rejoin(thread[0], NULL, K_FOREVER), "failed to join thread");

	/* At this point, there should be no more thread-specific values associated with any key */
	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(k_thread_key_delete(keys[i]), "attempt to delete keys[%d] failed", i);
	}
}

ZTEST_USER(threads_tss, test_key_resource_leak)
{
	void *key;

	for (size_t i = 0; i < 2 * CONFIG_THREAD_SPECIFIC_STORAGE_KEYS_MAX; ++i) {
		zassert_ok(k_thread_key_create(&key, NULL), "failed to create key %zu", i);
		zassert_ok(k_thread_key_delete(key), "failed to delete key %zu", i);
	}
}

ZTEST_USER(threads_tss, test_correct_key_is_deleted)
{
	size_t j = CONFIG_THREAD_SPECIFIC_STORAGE_KEYS_MAX - 1;

	for (size_t i = 0; i < ARRAY_SIZE(keys); ++i) {
		zassert_ok(k_thread_key_create(&keys[i], NULL), "failed to create key %zu", i);
	}

	void *key = keys[j];
	zassert_ok(k_thread_key_delete(keys[j]));
	zassert_ok(k_thread_key_create(&keys[j], NULL), "failed to create key %zu", j);

	zassert_equal(key, keys[j], "deleted key %p instead of key %p", keys[j], key);

	for (size_t i = 0; i < ARRAY_SIZE(keys); ++i) {
		zassert_ok(k_thread_key_delete(keys[i]), "failed to delete key %zu", i);
	}
}

ZTEST_USER(threads_tss, test_thread_specific_data_allocation)
{
	entry_mode = 3;

	for (int i = 0; i < N_THR; ++i) {
		k_thread_start(thread[i]);
		zassert_ok(k_thread_rejoin(thread[i], NULL, K_FOREVER));
	}

	zassert_equal(alloc_count[0], N_KEY / N_THR);

	for (int i = 1; i < N_THR; ++i) {
		zassert_equal(alloc_count[i - 1], alloc_count[i],
			      "uneven distribution of thread keys. alloc_count[%d]: %d != "
			      "alloc_count[%d]: %d",
			      i, alloc_count[i], i - 1, alloc_count[i - 1]);
	}

	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(k_thread_key_delete(keys[i]));
	}
}

static void tss_dtor(void *value)
{
	TC_PRINT("Thread %p: %s(%p)\n", k_current_get(), __func__, value);
	++dtor_count;
}

static void tss_dtor_store_again(void *value)
{
	++dtor_count;
	(void)k_thread_setspecific(keys[1], value);
}

static void make_keys_with_dtor(void)
{
	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(k_thread_key_create(&keys[i], tss_dtor));
	}
}

ZTEST_USER(threads_tss, test_thread_specific_data_destructor)
{
	/*
	 * In this test, we set thread-specific values in the child thread, but intentionally do not
	 * delete them, in order to verify that they are destroyed via graceful thread termination
	 * with k_thread_exit().
	 */
	make_keys_with_dtor();

	entry_mode = 4;

	for (int i = 0; i < N_THR; ++i) {
		k_thread_start(thread[i]);
		zassert_ok(k_thread_rejoin(thread[i], NULL, K_FOREVER));
	}

	zassert_equal(dtor_count, N_THR * N_KEY,
		      "tss destructor call mismatch. expected: %d, actual: %d", N_THR * N_KEY,
		      dtor_count);

	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(k_thread_key_delete(keys[i]));
	}

	/*
	 * A NULL value gets no destructor call, and a destructor that stores a value again is
	 * called once per pass, CONFIG_THREAD_SPECIFIC_STORAGE_DESTRUCTOR_ITERATIONS times, after
	 * which the thread still exits.
	 */
	zassert_ok(k_thread_key_create(&keys[0], tss_dtor));
	zassert_ok(k_thread_key_create(&keys[1], tss_dtor_store_again));
	dtor_count = 0;
	entry_mode = 6;
	k_thread_start(thread[0]);
	zassert_ok(k_thread_rejoin(thread[0], NULL, K_FOREVER));
	zassert_equal(dtor_count, CONFIG_THREAD_SPECIFIC_STORAGE_DESTRUCTOR_ITERATIONS,
		      "destructor called %d times", dtor_count);
	for (int i = 0; i < N_KEY; i++) {
		zassert_ok(k_thread_key_delete(keys[i]));
	}
}

ZTEST_USER(threads_tss, test_k_thread_key_delete)
{
	void *value = NULL;

	zassert_equal(k_thread_key_delete((void *)0x1), -EINVAL);

	/* deleting a key drops a value another thread still holds, without its destructor */
	zassert_ok(k_thread_key_create(&keys[0], tss_dtor));
	dtor_count = 0;
	entry_mode = 5;
	k_thread_start(thread[0]);
	k_sem_take(&tss_held, K_FOREVER);
	zassert_ok(k_thread_key_delete(keys[0]));
	k_sem_give(&tss_release);
	zassert_ok(k_thread_rejoin(thread[0], NULL, K_FOREVER));
	zassert_equal(dtor_count, 0, "destructor ran for a deleted key");

	/* the calling thread's own value goes the same way, and the slot is free again */
	zassert_ok(k_thread_key_create(&keys[0], tss_dtor));
	zassert_ok(k_thread_setspecific(keys[0], (void *)0x42));
	zassert_ok(k_thread_key_delete(keys[0]));
	zassert_equal(k_thread_getspecific(keys[0], &value), -EINVAL);
	zassert_ok(k_thread_key_create(&keys[1], NULL));
	zassert_equal(keys[0], keys[1]);
	zassert_equal(k_thread_getspecific(keys[1], &value), -EINVAL);
	zassert_ok(k_thread_key_delete(keys[1]));
	zassert_equal(dtor_count, 0);
}

void threads_tss_before(void *arg)
{
	ARG_UNUSED(arg);

	for (int i = 0; i < N_THR; ++i) {
#ifdef CONFIG_USERSPACE
		/* grant ztest_thread permissions on thread[i] */
		k_thread_access_grant(k_current_get(), thread[i], &tss_held, &tss_release);
		k_thread_access_grant(thread[i], &tss_held, &tss_release);
#endif

		/* So statically-defined userspace threads can be restarted with k_thread_start() */
		k_object_init(thread[i]);
		k_thread_create(thread[i], thread_stack[i], CONFIG_ZTEST_STACK_SIZE, entry, NULL,
				NULL, NULL, K_PRIO_PREEMPT(10), K_USER, K_FOREVER);
	}
}

ZTEST_SUITE(threads_tss, NULL, NULL, threads_tss_before, NULL, NULL);
