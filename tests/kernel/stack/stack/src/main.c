/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file
 * @brief Use stack API's in different scenarios
 *
 * This module tests following three basic scenarios:
 *
 * Scenario #1
 * Test thread enters items into a stack, starts the Child thread and
 * waits for a semaphore. Child thread extracts all items from the stack
 * and enters some items back into the stack. Child thread gives the
 * semaphore for Test thread to continue. Once the control is returned
 * back to Test thread, it extracts all items from the stack.
 *
 * Scenario #2
 * Test thread enters an item into stack2, starts a Child thread and
 * extract an item from stack1 once the item is there. The child thread
 * will extract an item from stack2 once the item is there and and enter
 * an item to stack1. The flow of control goes from Test thread to Child
 * thread and so forth.
 *
 * Scenario #3
 * Tests the ISR interfaces. Test thread pushes items into stack2 and gives
 * control to the Child thread. Child thread pops items from stack2 and then
 * pushes items into stack1. Child thread gives back control to the Test thread
 * and Test thread pops the items from stack1.
 * All the Push and Pop operations happen in ISR Context.
 */


/**
 * @brief Tests for Kernel stack objects
 * @defgroup kernel_stack_tests Stacks
 * @ingroup all_tests
 * @{
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

#define TSTACK_SIZE     (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define STACK_LEN       4

/* stack objects used in this test */
K_STACK_DEFINE(stack1, STACK_LEN);
K_STACK_DEFINE(stack2, STACK_LEN);

/* thread info * */
K_THREAD_STACK_DEFINE(threadstack, TSTACK_SIZE);
struct k_thread thread_data;

/* Data pushed to stack */
static ZTEST_DMEM stack_data_t data1[STACK_LEN] = { 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD };
static ZTEST_DMEM stack_data_t data2[STACK_LEN] = { 0x1111, 0x2222, 0x3333, 0x4444 };
static ZTEST_DMEM stack_data_t data_isr[STACK_LEN] = { 0xABCD, 0xABCD, 0xABCD,
						       0xABCD };

/* semaphore to sync threads */
static struct k_sem end_sema;



K_MEM_POOL_DEFINE(test_pool, 128, 128, 2, 4);

extern struct k_stack kstack;
extern struct k_stack stack;
extern struct k_sem end_sema;

extern void test_stack_thread2thread(void);
extern void test_stack_thread2isr(void);
extern void test_stack_pop_fail(void);
extern void test_stack_alloc_thread2thread(void);
extern void test_stack_pop_can_wait(void);
#ifdef CONFIG_USERSPACE
extern void test_stack_user_thread2thread(void);
extern void test_stack_user_pop_fail(void);
#else
#define dummy_test(_name)	   \
	static void _name(void)	   \
	{			   \
		ztest_test_skip(); \
	}

dummy_test(test_stack_user_thread2thread);
dummy_test(test_stack_user_pop_fail);
#endif /* CONFIG_USERSPACE */

/* entry of contexts */
static void tIsr_entry_push(const void *p)
{
	uint32_t i;

	/* Push items to stack */
	for (i = 0U; i < STACK_LEN; i++) {
		k_stack_push((struct k_stack *)p, data_isr[i]);
	}
}

static void tIsr_entry_pop(const void *p)
{
	uint32_t i;

	/* Pop items from stack */
	for (i = 0U; i < STACK_LEN; i++) {
		if (p == &stack1) {
			k_stack_pop((struct k_stack *)p, &data1[i], K_NO_WAIT);
		} else {
			k_stack_pop((struct k_stack *)p, &data2[i], K_NO_WAIT);
		}
	}
}

static void thread_entry_fn_single(void *p1, void *p2, void *p3)
{
	stack_data_t tmp[STACK_LEN];
	uint32_t i;

	/* Pop items from stack */
	for (i = STACK_LEN; i; i--) {
		k_stack_pop((struct k_stack *)p1, &tmp[i - 1], K_NO_WAIT);
	}
	zassert_false(memcmp(tmp, data1, sizeof(tmp)),
		      "Push & Pop items does not match");

	/* Push items from stack */
	for (i = 0U; i < STACK_LEN; i++) {
		k_stack_push((struct k_stack *)p1, data2[i]);
	}

	/* Give control back to Test thread */
	k_sem_give(&end_sema);
}

static void thread_entry_fn_dual(void *p1, void *p2, void *p3)
{
	stack_data_t tmp[STACK_LEN];
	uint32_t i;

	for (i = 0U; i < STACK_LEN; i++) {
		/* Pop items from stack2 */
		k_stack_pop(p2, &tmp[i], K_FOREVER);

		/* Push items to stack1 */
		k_stack_push(p1, data1[i]);

	}
	zassert_false(memcmp(tmp, data2, sizeof(tmp)),
		      "Push & Pop items does not match");
}

static void thread_entry_fn_isr(void *p1, void *p2, void *p3)
{
	/* Pop items from stack2 */
	irq_offload(tIsr_entry_pop, (const void *)p2);
	zassert_false(memcmp(data_isr, data2, sizeof(data_isr)),
		      "Push & Pop items does not match");

	/* Push items to stack1 */
	irq_offload(tIsr_entry_push, (const void *)p1);

	/* Give control back to Test thread */
	k_sem_give(&end_sema);
}

/**
 * @addtogroup kernel_stack_tests
 * @{
 */

/**
 * @brief Verify data passing between threads using single stack
 * @see k_stack_push(), #K_STACK_DEFINE(x), k_stack_pop()
 */
static void test_single_stack_play(void)
{
	stack_data_t tmp[STACK_LEN];
	uint32_t i;

	/* Init kernel objects */
	k_sem_init(&end_sema, 0, 1);

	/* Push items to stack */
	for (i = 0U; i < STACK_LEN; i++) {
		k_stack_push(&stack1, data1[i]);
	}

	k_tid_t tid = k_thread_create(&thread_data, threadstack, TSTACK_SIZE,
				      thread_entry_fn_single, &stack1, NULL,
				      NULL, K_PRIO_PREEMPT(0), K_USER |
				      K_INHERIT_PERMS, K_NO_WAIT);

	/* Let the child thread run */
	k_sem_take(&end_sema, K_FOREVER);

	/* Pop items from stack */
	for (i = STACK_LEN; i; i--) {
		k_stack_pop(&stack1, &tmp[i - 1], K_NO_WAIT);
	}

	zassert_false(memcmp(tmp, data2, sizeof(tmp)),
		      "Push & Pop items does not match");

	/* Clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
}

/**
 * @brief Verify data passing between threads using dual stack
 * @see k_stack_push(), #K_STACK_DEFINE(x), k_stack_pop()
 */
static void test_dual_stack_play(void)
{
	stack_data_t tmp[STACK_LEN];
	uint32_t i;

	k_tid_t tid = k_thread_create(&thread_data, threadstack, TSTACK_SIZE,
				      thread_entry_fn_dual, &stack1, &stack2,
				      NULL, K_PRIO_PREEMPT(0), K_USER |
				      K_INHERIT_PERMS, K_NO_WAIT);

	for (i = 0U; i < STACK_LEN; i++) {
		/* Push items to stack2 */
		k_stack_push(&stack2, data2[i]);

		/* Pop items from stack1 */
		k_stack_pop(&stack1, &tmp[i], K_FOREVER);
	}

	zassert_false(memcmp(tmp, data1, sizeof(tmp)),
		      "Push & Pop items does not match");

	/* Clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
}

/**
 * @brief Verify data passing between thread and ISR
 * @see k_stack_push(), #K_STACK_DEFINE(x), k_stack_pop()
 */
static void test_isr_stack_play(void)
{
	/* Init kernel objects */
	k_sem_init(&end_sema, 0, 1);

	k_tid_t tid = k_thread_create(&thread_data, threadstack, TSTACK_SIZE,
				      thread_entry_fn_isr, &stack1, &stack2,
				      NULL, K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS, K_NO_WAIT);


	/* Push items to stack2 */
	irq_offload(tIsr_entry_push, (const void *)&stack2);

	/* Let the child thread run */
	k_sem_take(&end_sema, K_FOREVER);

	/* Pop items from stack1 */
	irq_offload(tIsr_entry_pop, (const void *)&stack1);

	zassert_false(memcmp(data_isr, data1, sizeof(data_isr)),
		      "Push & Pop items does not match");

	/* Clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
}

/* the thread entry */
void thread_entry_wait(void *p1, void *p2, void *p3)
{
	stack_data_t *txdata = p3;

	k_stack_push(p1, *(txdata + 2));
	k_stack_push(p1, *(txdata + 3));
}

/**
 * @brief Test that the stack pop can be waited
 * if no item availablle
 *
 * @details Create and initialize a new stack
 * Set two timeout parameters to indicate
 * the maximum amount of time the thread will wait.
 *
 * @ingroup kernel_stack_tests
 *
 * @see k_stack_push(), #K_STACK_DEFINE(x), k_stack_pop()
 */
void test_stack_pop_can_wait(void)
{
	struct k_stack stack3;
	stack_data_t tx_data[STACK_LEN] = { 0xaa, 0xbb, 0xcc, 0xdd };
	stack_data_t rx_data[STACK_LEN] = { 0 };

	k_stack_alloc_init(&stack3, 2);
	k_tid_t tid = k_thread_create(&thread_data, threadstack,
			TSTACK_SIZE, thread_entry_wait, &stack3,
			NULL, tx_data, K_PRIO_PREEMPT(0), 0,
			K_NO_WAIT);

	for (int i = 0; i < 2; i++) {
		k_stack_push(&stack3, tx_data[i]);
	}

	for (int i = 0; i < 3; i++) {
		k_stack_pop(&stack3, &rx_data[i], K_FOREVER);
	}

	zassert_true(rx_data[2] == tx_data[2], "wait foreve and pop failed\n");
	k_stack_pop(&stack3, &rx_data[3], K_MSEC(50));
	zassert_true(rx_data[3] == tx_data[3], "Wait maxmum time pop failed\n");
	/* Clear the spawn thread to avoid side effect */
	k_thread_abort(tid);
	/*free the buffer allocated*/
	k_stack_cleanup(&stack3);
}

/**
 * @}
 */

extern struct k_stack threadstack1;
extern struct k_thread thread_data1;
extern struct k_sem end_sema1;

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &stack1, &stack2, &thread_data,
			      &end_sema, &threadstack, &kstack, &stack, &thread_data1,
			      &end_sema1, &threadstack1);

	k_thread_resource_pool_assign(k_current_get(), &test_pool);

	ztest_test_suite(test_stack_usage,
			 ztest_unit_test(test_stack_thread2thread),
			 ztest_user_unit_test(test_stack_user_thread2thread),
			 ztest_unit_test(test_stack_thread2isr),
			 ztest_unit_test(test_stack_pop_fail),
			 ztest_user_unit_test(test_stack_user_pop_fail),
			 ztest_unit_test(test_stack_alloc_thread2thread),
			 ztest_user_unit_test(test_single_stack_play),
			 ztest_1cpu_user_unit_test(test_dual_stack_play),
			 ztest_1cpu_unit_test(test_isr_stack_play),
			 ztest_unit_test(test_stack_pop_can_wait));
	ztest_run_test_suite(test_stack_usage);
}
