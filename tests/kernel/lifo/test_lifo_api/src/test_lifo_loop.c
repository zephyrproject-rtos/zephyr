/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_lifo_api
 * @{
 * @defgroup t_lifo_loop test_lifo_loop
 * @brief TestPurpose: verify zephyr lifo continuous read write
 *                     in loop
 * @details
 * - Test Steps
 *   -# lifo put from main thread
 *   -# lifo read from isr
 *   -# lifo put from isr
 *   -# lifo get from spawn thread
 *   -# loop above steps for LOOPs times
 * - Expected Results
 *   -# lifo data pass correctly and stably across contexts
 * - API coverage
 *   -# k_lifo_init
 *   -# k_lifo_put
 *   -# k_lifo_get
 * @}
 */

#include "test_lifo.h"

#define STACK_SIZE 512
#define LIST_LEN 4
#define LOOPS 32

static ldata_t data[LIST_LEN];
static struct k_lifo lifo;
static char __noinit __stack tstack[STACK_SIZE];
static struct k_sem end_sema;

static void tlifo_put(struct k_lifo *plifo)
{
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: lifo put*/
		k_lifo_put(plifo, (void *)&data[i]);
	}
}

static void tlifo_get(struct k_lifo *plifo)
{
	void *rx_data;

	/*get lifo data*/
	for (int i = LIST_LEN-1; i >= 0; i--) {
		/**TESTPOINT: lifo get*/
		rx_data = k_lifo_get(plifo, K_FOREVER);
		assert_equal(rx_data, (void *)&data[i], NULL);
	}
}

/*entry of contexts*/
static void tIsr_entry(void *p)
{
	TC_PRINT("isr lifo get\n");
	tlifo_get((struct k_lifo *)p);
	TC_PRINT("isr lifo put ---> ");
	tlifo_put((struct k_lifo *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	TC_PRINT("thread lifo get\n");
	tlifo_get((struct k_lifo *)p1);
	k_sem_give(&end_sema);
	TC_PRINT("thread lifo put ---> ");
	tlifo_put((struct k_lifo *)p1);
	k_sem_give(&end_sema);
}

/* lifo read write job */
static void tlifo_read_write(struct k_lifo *plifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr-thread data passing via lifo*/
	k_tid_t tid = k_thread_spawn(tstack, STACK_SIZE,
		tThread_entry, plifo, NULL, NULL,
		K_PRIO_PREEMPT(0), 0, 0);

	TC_PRINT("main lifo put ---> ");
	tlifo_put(plifo);
	irq_offload(tIsr_entry, plifo);
	k_sem_take(&end_sema, K_FOREVER);
	k_sem_take(&end_sema, K_FOREVER);

	TC_PRINT("main lifo get\n");
	tlifo_get(plifo);
	k_thread_abort(tid);
	TC_PRINT("\n");
}

/*test cases*/
void test_lifo_loop(void)
{
	k_lifo_init(&lifo);
	for (int i = 0; i < LOOPS; i++) {
		TC_PRINT("* Pass data by lifo in loop %d\n", i);
		tlifo_read_write(&lifo);
	}
}
