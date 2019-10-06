/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define STACK_LEN 4

/**TESTPOINT: init via K_STACK_DEFINE*/
K_STACK_DEFINE(kstack, STACK_LEN);
K_STACK_DEFINE(kstack_test_alloc, STACK_LEN);
struct k_stack stack;

K_THREAD_STACK_DEFINE(threadstack, STACK_SIZE);
struct k_thread thread_data;
static ZTEST_DMEM stack_data_t data[STACK_LEN] = { 0xABCD, 0x1234 };
struct k_sem end_sema;

static void tstack_push(struct k_stack *pstack)
{
	for (int i = 0; i < STACK_LEN; i++) {
		/**TESTPOINT: stack push*/
		k_stack_push(pstack, data[i]);
	}
}

static void tstack_pop(struct k_stack *pstack)
{
	stack_data_t rx_data;

	for (int i = STACK_LEN - 1; i >= 0; i--) {
		/**TESTPOINT: stack pop*/
		zassert_false(k_stack_pop(pstack, &rx_data, K_NO_WAIT), NULL);
		zassert_equal(rx_data, data[i], NULL);
	}
}

/*entry of contexts*/
static void tIsr_entry_push(void *p)
{
	tstack_push((struct k_stack *)p);
}

static void tIsr_entry_pop(void *p)
{
	tstack_pop((struct k_stack *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tstack_pop((struct k_stack *)p1);
	k_sem_give(&end_sema);
	tstack_push((struct k_stack *)p1);
	k_sem_give(&end_sema);
}

static void tstack_thread_thread(struct k_stack *pstack)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-thread data passing via stack*/
	k_tid_t tid = k_thread_create(&thread_data, threadstack, STACK_SIZE,
				      tThread_entry, pstack, NULL, NULL,
				      K_PRIO_PREEMPT(0), K_USER |
				      K_INHERIT_PERMS, K_NO_WAIT);
	tstack_push(pstack);
	k_sem_take(&end_sema, K_FOREVER);

	k_sem_take(&end_sema, K_FOREVER);
	tstack_pop(pstack);

	/* clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
}

static void tstack_thread_isr(struct k_stack *pstack)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr data passing via stack*/
	irq_offload(tIsr_entry_push, pstack);
	tstack_pop(pstack);

	tstack_push(pstack);
	irq_offload(tIsr_entry_pop, pstack);
}

/**
 * @addtogroup kernel_stack_tests
 * @{
 */

/**
 * @brief Test to verify data passing between threads via stack
 * @see k_stack_init(), k_stack_push(), #K_STACK_DEFINE(x), k_stack_pop()
 */
void test_stack_thread2thread(void)
{
	/**TESTPOINT: test k_stack_init stack*/
	k_stack_init(&stack, data, STACK_LEN);
	tstack_thread_thread(&stack);

	/**TESTPOINT: test K_STACK_DEFINE stack*/
	tstack_thread_thread(&kstack);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Verifies data passing between user threads via stack
 * @see k_stack_init(), k_stack_push(), #K_STACK_DEFINE(x), k_stack_pop()
 */
void test_stack_user_thread2thread(void)
{
	struct k_stack *stack = k_object_alloc(K_OBJ_STACK);

	zassert_not_null(stack, "couldn't allocate stack object");
	zassert_false(k_stack_alloc_init(stack, STACK_LEN),
		      "stack init failed");

	tstack_thread_thread(stack);
}
#endif

/**
 * @brief Verifies data passing between thread and ISR via stack
 * @see k_stack_init(), k_stack_push(), #K_STACK_DEFINE(x), k_stack_pop()
 */
void test_stack_thread2isr(void)
{
	/**TESTPOINT: test k_stack_init stack*/
	k_stack_init(&stack, data, STACK_LEN);
	tstack_thread_isr(&stack);

	/**TESTPOINT: test K_STACK_DEFINE stack*/
	tstack_thread_isr(&kstack);
}

/**
 * @see k_stack_alloc_init(), k_stack_push(), #K_STACK_DEFINE(x), k_stack_pop(),
 * k_stack_cleanup()
 */
void test_stack_alloc_thread2thread(void)
{
	int ret;

	k_stack_alloc_init(&kstack_test_alloc, STACK_LEN);

	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-thread data passing via stack*/
	k_tid_t tid = k_thread_create(&thread_data, threadstack, STACK_SIZE,
					tThread_entry, &kstack_test_alloc,
					NULL, NULL, K_PRIO_PREEMPT(0), 0,
					K_NO_WAIT);
	tstack_push(&kstack_test_alloc);
	k_sem_take(&end_sema, K_FOREVER);

	k_sem_take(&end_sema, K_FOREVER);
	tstack_pop(&kstack_test_alloc);

	/* clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
	k_stack_cleanup(&kstack_test_alloc);

	/** Requested buffer allocation from the test pool.*/
	ret = k_stack_alloc_init(&kstack_test_alloc, (STACK_SIZE/2)+1);
	zassert_true(ret == -ENOMEM,
			"resource pool is smaller then requested buffer");
}

/**
 * @}
 */
