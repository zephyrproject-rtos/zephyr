/*
 * Copyright (c) 2022, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define TIMEOUT_MS 500

#define POOL_SIZE 20480

#ifdef CONFIG_USERSPACE
#define STACK_OBJ_SIZE K_THREAD_STACK_LEN(CONFIG_DYNAMIC_THREAD_STACK_SIZE)
#else
#define STACK_OBJ_SIZE K_KERNEL_STACK_LEN(CONFIG_DYNAMIC_THREAD_STACK_SIZE)
#endif

#define MAX_HEAP_STACKS (POOL_SIZE / STACK_OBJ_SIZE)

K_HEAP_DEFINE(stack_heap, POOL_SIZE);

ZTEST_DMEM bool tflag[MAX(CONFIG_DYNAMIC_THREAD_POOL_SIZE, MAX_HEAP_STACKS)];

static void func(void *arg1, void *arg2, void *arg3)
{
	bool *flag = (bool *)arg1;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	printk("Hello, dynamic world!\n");

	*flag = true;
}

/** @brief Check we can create a thread from userspace, using dynamic objects */
ZTEST_USER(dynamic_thread_stack, test_dynamic_thread_stack_userspace_dyn_obj)
{
	k_tid_t tid;
	struct k_thread *th;
	k_thread_stack_t *stack;

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		ztest_test_skip();
	}

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_ALLOC)) {
		ztest_test_skip();
	}

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
		ztest_test_skip();
	}

	stack = k_thread_stack_alloc(CONFIG_DYNAMIC_THREAD_STACK_SIZE, K_USER);
	zassert_not_null(stack);

	th = k_object_alloc(K_OBJ_THREAD);
	zassert_not_null(th);

	tid = k_thread_create(th, stack, CONFIG_DYNAMIC_THREAD_STACK_SIZE, func,
			      &tflag[0], NULL, NULL, 0,
			      K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	zassert_not_null(tid);

	zassert_ok(k_thread_join(tid, K_MSEC(TIMEOUT_MS)));
	zassert_true(tflag[0]);
	zassert_ok(k_thread_stack_free(stack));
}

/** @brief Exercise the pool-based thread stack allocator */
ZTEST(dynamic_thread_stack, test_dynamic_thread_stack_pool)
{
	static k_tid_t tid[CONFIG_DYNAMIC_THREAD_POOL_SIZE];
	static struct k_thread th[CONFIG_DYNAMIC_THREAD_POOL_SIZE];
	static k_thread_stack_t *stack[CONFIG_DYNAMIC_THREAD_POOL_SIZE];

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_POOL)) {
		ztest_test_skip();
	}

	/* allocate all thread stacks from the pool */
	for (size_t i = 0; i < CONFIG_DYNAMIC_THREAD_POOL_SIZE; ++i) {
		stack[i] = k_thread_stack_alloc(CONFIG_DYNAMIC_THREAD_STACK_SIZE,
						IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0);

		zassert_not_null(stack[i]);
	}

	if (IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
		/* ensure 1 thread can be allocated from the heap when the pool is depleted */
		zassert_ok(k_thread_stack_free(
			k_thread_stack_alloc(CONFIG_DYNAMIC_THREAD_STACK_SIZE,
					     IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0)));
	} else {
		/* ensure that no more thread stacks can be allocated from the pool */
		zassert_is_null(k_thread_stack_alloc(CONFIG_DYNAMIC_THREAD_STACK_SIZE,
						     IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0));
	}

	/* spawn our threads */
	for (size_t i = 0; i < CONFIG_DYNAMIC_THREAD_POOL_SIZE; ++i) {
		tflag[i] = false;
		tid[i] = k_thread_create(&th[i], stack[i],
				CONFIG_DYNAMIC_THREAD_STACK_SIZE, func,
				&tflag[i], NULL, NULL, 0,
				K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* join all threads and check that flags have been set */
	for (size_t i = 0; i < CONFIG_DYNAMIC_THREAD_POOL_SIZE; ++i) {
		zassert_ok(k_thread_join(tid[i], K_MSEC(TIMEOUT_MS)));
		zassert_true(tflag[i]);
	}

	/* clean up stacks allocated from the pool */
	for (size_t i = 0; i < CONFIG_DYNAMIC_THREAD_POOL_SIZE; ++i) {
		zassert_ok(k_thread_stack_free(stack[i]));
	}
}

/** @brief Exercise the heap-based thread stack allocator */
ZTEST(dynamic_thread_stack, test_dynamic_thread_stack_alloc)
{
	size_t N;
	static k_tid_t tid[MAX_HEAP_STACKS];
	static struct k_thread th[MAX_HEAP_STACKS];
	static k_thread_stack_t *stack[MAX_HEAP_STACKS];

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_PREFER_ALLOC)) {
		ztest_test_skip();
	}

	if (!IS_ENABLED(CONFIG_DYNAMIC_THREAD_ALLOC)) {
		ztest_test_skip();
	}

	/* allocate all thread stacks from the heap */
	for (N = 0; N < MAX_HEAP_STACKS; ++N) {
		stack[N] = k_thread_stack_alloc(CONFIG_DYNAMIC_THREAD_STACK_SIZE,
						IS_ENABLED(CONFIG_USERSPACE) ? K_USER : 0);
		if (stack[N] == NULL) {
			break;
		}
	}

	/* spwan our threads */
	for (size_t i = 0; i < N; ++i) {
		tflag[i] = false;
		tid[i] = k_thread_create(&th[i], stack[i],
					 CONFIG_DYNAMIC_THREAD_STACK_SIZE, func,
					 &tflag[i], NULL, NULL, 0,
					 K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	/* join all threads and check that flags have been set */
	for (size_t i = 0; i < N; ++i) {
		zassert_ok(k_thread_join(tid[i], K_MSEC(TIMEOUT_MS)));
		zassert_true(tflag[i]);
	}

	/* clean up stacks allocated from the heap */
	for (size_t i = 0; i < N; ++i) {
		zassert_ok(k_thread_stack_free(stack[i]));
	}
}

static void *dynamic_thread_stack_setup(void)
{
	k_thread_heap_assign(k_current_get(), &stack_heap);
	return NULL;
}

ZTEST_SUITE(dynamic_thread_stack, NULL, dynamic_thread_stack_setup, NULL, NULL, NULL);
