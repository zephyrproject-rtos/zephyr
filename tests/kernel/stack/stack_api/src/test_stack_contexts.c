/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_stack_api
 * @{
 * @defgroup t_stack_api_basic test_stack_api_basic
 * @brief TestPurpose: verify zephyr stack apis under different context
 * - API coverage
 *   -# k_stack_init K_STACK_DEFINE
 *   -# k_stack_push
 *   -# k_stack_pop
 * @}
 */

#include <ztest.h>
#include <irq_offload.h>

#define STACK_SIZE 512
#define STACK_LEN 2

/**TESTPOINT: init via K_STACK_DEFINE*/
K_STACK_DEFINE(kstack, STACK_LEN);
static struct k_stack stack;

static K_THREAD_STACK_DEFINE(threadstack, STACK_SIZE);
static struct k_thread thread_data;
static u32_t data[STACK_LEN] = { 0xABCD, 0x1234 };
static struct k_sem end_sema;

static void tstack_push(struct k_stack *pstack)
{
	for (int i = 0; i < STACK_LEN; i++) {
		/**TESTPOINT: stack push*/
		k_stack_push(pstack, data[i]);
	}
}

static void tstack_pop(struct k_stack *pstack)
{
	u32_t rx_data;

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
				      K_PRIO_PREEMPT(0), 0, 0);
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

/*test cases*/
void test_stack_thread2thread(void)
{
	/**TESTPOINT: test k_stack_init stack*/
	k_stack_init(&stack, data, STACK_LEN);
	tstack_thread_thread(&stack);

	/**TESTPOINT: test K_STACK_INIT stack*/
	tstack_thread_thread(&kstack);
}

void test_stack_thread2isr(void)
{
	/**TESTPOINT: test k_stack_init stack*/
	k_stack_init(&stack, data, STACK_LEN);
	tstack_thread_isr(&stack);

	/**TESTPOINT: test K_STACK_INIT stack*/
	tstack_thread_isr(&kstack);
}
