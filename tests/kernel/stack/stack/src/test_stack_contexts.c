/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/irq_offload.h>
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define STACK_LEN 4
#define HIGH_T1			0xaaa
#define HIGH_T2			0xbbb
#define LOW_PRIO			0xccc

/**TESTPOINT: init via K_STACK_DEFINE*/
K_STACK_DEFINE(kstack, STACK_LEN);
K_STACK_DEFINE(kstack_test_alloc, STACK_LEN);
struct k_stack stack;

K_THREAD_STACK_DEFINE(threadstack1, STACK_SIZE);
struct k_thread thread_data1;
K_THREAD_STACK_DEFINE(threadstack_t1, STACK_SIZE);
static struct k_thread high_pro_thread_t1;
K_THREAD_STACK_DEFINE(threadstack_t2, STACK_SIZE);
static struct k_thread high_pro_thread_t2;
static ZTEST_DMEM stack_data_t data[STACK_LEN] = { 0xABCD, 0x1234 };
struct k_sem end_sema1;

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
static void tIsr_entry_push(const void *p)
{
	tstack_push((struct k_stack *)p);
}

static void tIsr_entry_pop(const void *p)
{
	tstack_pop((struct k_stack *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tstack_pop((struct k_stack *)p1);
	k_sem_give(&end_sema1);
	tstack_push((struct k_stack *)p1);
	k_sem_give(&end_sema1);
}

static void tstack_thread_thread(struct k_stack *pstack)
{
	k_sem_init(&end_sema1, 0, 1);
	/**TESTPOINT: thread-thread data passing via stack*/
	k_tid_t tid = k_thread_create(&thread_data1, threadstack1, STACK_SIZE,
				      tThread_entry, pstack, NULL, NULL,
				      K_PRIO_PREEMPT(0), K_USER |
				      K_INHERIT_PERMS, K_NO_WAIT);
	tstack_push(pstack);
	k_sem_take(&end_sema1, K_FOREVER);

	k_sem_take(&end_sema1, K_FOREVER);
	tstack_pop(pstack);

	/* clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
}

static void tstack_thread_isr(struct k_stack *pstack)
{
	k_sem_init(&end_sema1, 0, 1);
	/**TESTPOINT: thread-isr data passing via stack*/
	irq_offload(tIsr_entry_push, (const void *)pstack);
	tstack_pop(pstack);

	tstack_push(pstack);
	irq_offload(tIsr_entry_pop, (const void *)pstack);
}

/**
 * @addtogroup kernel_stack_tests
 * @{
 */

/**
 * @brief Test to verify data passing between threads via stack
 *
 * @details Static define and Dynamic define stacks,
 * Then initialize them.
 * Current thread push or pop data item into the stack.
 * Create a new thread pop or push data item into the stack.
 * Controlled by semaphore.
 * Verify data passing between threads via stack
 * And verify stack can be define at compile time.
 *
 * @ingroup kernel_stack_tests
 *
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

	k_sem_init(&end_sema1, 0, 1);
	/**TESTPOINT: thread-thread data passing via stack*/
	k_tid_t tid = k_thread_create(&thread_data1, threadstack1, STACK_SIZE,
					tThread_entry, &kstack_test_alloc,
					NULL, NULL, K_PRIO_PREEMPT(0), 0,
					K_NO_WAIT);
	tstack_push(&kstack_test_alloc);
	k_sem_take(&end_sema1, K_FOREVER);

	k_sem_take(&end_sema1, K_FOREVER);
	tstack_pop(&kstack_test_alloc);

	/* clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
	k_stack_cleanup(&kstack_test_alloc);

	/** Requested buffer allocation from the test pool.*/
	ret = k_stack_alloc_init(&kstack_test_alloc, (STACK_SIZE/2)+1);
	zassert_true(ret == -ENOMEM,
			"requested buffer is smaller than resource pool");
}

static void low_prio_wait_for_stack(void *p1, void *p2, void *p3)
{
	struct k_stack *pstack = p1;
	stack_data_t output;

	k_stack_pop(pstack, &output, K_FOREVER);
	zassert_true(output == LOW_PRIO,
	    "The low priority thread get the stack data failed lastly");
}

static void high_prio_t1_wait_for_stack(void *p1, void *p2, void *p3)
{
	struct k_stack *pstack = p1;
	stack_data_t output;

	k_stack_pop(pstack, &output, K_FOREVER);
	zassert_true(output == HIGH_T1,
	    "The highest priority and waited longest get the stack data failed firstly");
}

static void high_prio_t2_wait_for_stack(void *p1, void *p2, void *p3)
{
	struct k_stack *pstack = p1;
	stack_data_t output;

	k_stack_pop(pstack, &output, K_FOREVER);
	zassert_true(output == HIGH_T2,
	   "The higher priority and waited longer get the stack data failed secondly");
}

/**
 * @brief Test multi-threads to get data from stack.
 *
 * @details Define three threads, and set a higher priority for two of them,
 * and set a lower priority for the last one. Then Add a delay between
 * creating the two high priority threads.
 * Test point:
 * 1. Any number of threads may wait(K_FOREVER set) on an empty stack
 * simultaneously.
 * 2. When data is pushed, it is given to the highest priority
 * thread that has waited longest.
 *
 * @ingroup kernel_stack_tests
 */
void test_stack_multithread_competition(void)
{
	int old_prio = k_thread_priority_get(k_current_get());
	int prio = 10;
	stack_data_t test_data[3];

	memset(test_data, 0, sizeof(test_data));
	k_thread_priority_set(k_current_get(), prio);

	/* Set up some values */
	test_data[0] = HIGH_T1;
	test_data[1] = HIGH_T2;
	test_data[2] = LOW_PRIO;

	k_thread_create(&thread_data1, threadstack1, STACK_SIZE,
			low_prio_wait_for_stack,
			&stack, NULL, NULL,
			prio + 4, 0, K_NO_WAIT);

	k_thread_create(&high_pro_thread_t1, threadstack_t1, STACK_SIZE,
			high_prio_t1_wait_for_stack,
			&stack, NULL, NULL,
			prio + 2, 0, K_NO_WAIT);

	/* Make thread thread_data1 and high_pro_thread_t1 wait more time */
	k_sleep(K_MSEC(10));

	k_thread_create(&high_pro_thread_t2, threadstack_t2, STACK_SIZE,
			high_prio_t2_wait_for_stack,
			&stack, NULL, NULL,
			prio + 2, 0, K_NO_WAIT);

	/* Initialize them and block */
	k_sleep(K_MSEC(50));

	/* Insert some data to wake up thread */
	k_stack_push(&stack, test_data[0]);
	k_stack_push(&stack, test_data[1]);
	k_stack_push(&stack, test_data[2]);

	/* Wait for thread exiting */
	k_thread_join(&thread_data1, K_FOREVER);
	k_thread_join(&high_pro_thread_t1, K_FOREVER);
	k_thread_join(&high_pro_thread_t2, K_FOREVER);

	/* Revert priority of the main thread */
	k_thread_priority_set(k_current_get(), old_prio);
}

/**
 * @brief Test case of requesting a buffer larger than resource pool.
 *
 * @details Try to request a buffer larger than resource pool for stack,
 * then see if returns an expected value.
 *
 * @ingroup kernel_stack_tests
 */
void test_stack_alloc_null(void)
{
	int ret;

	/* Requested buffer allocation from the test pool. */
	ret = k_stack_alloc_init(&kstack_test_alloc, (STACK_SIZE/2)+1);
	zassert_true(ret == -ENOMEM,
			"requested buffer is smaller than resource pool");
}

/**
 * @}
 */
