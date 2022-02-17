/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_fifo.h"

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define LIST_LEN 4
#define LOOPS 32

static fdata_t data[LIST_LEN];
static struct k_fifo fifo;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static struct k_sem end_sema;

static void tfifo_put(struct k_fifo *pfifo)
{
	/**TESTPOINT: fifo put*/
	for (int i = 0; i < LIST_LEN; i++) {
		k_fifo_put(pfifo, (void *)&data[i]);
	}
}

static void tfifo_get(struct k_fifo *pfifo)
{
	void *rx_data;

	/*get fifo data from "fifo_put"*/
	for (int i = 0; i < LIST_LEN; i++) {
		/**TESTPOINT: fifo get*/
		rx_data = k_fifo_get(pfifo, K_NO_WAIT);
		zassert_equal(rx_data, (void *)&data[i], NULL);
	}
}

/*entry of contexts*/
static void tIsr_entry(const void *p)
{
	TC_PRINT("isr fifo get\n");
	tfifo_get((struct k_fifo *)p);
	TC_PRINT("isr fifo put ---> ");
	tfifo_put((struct k_fifo *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	TC_PRINT("thread fifo get\n");
	tfifo_get((struct k_fifo *)p1);
	k_sem_give(&end_sema);
	TC_PRINT("thread fifo put ---> ");
	tfifo_put((struct k_fifo *)p1);
	k_sem_give(&end_sema);
}

/* fifo read write job */
static void tfifo_read_write(struct k_fifo *pfifo)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-isr-thread data passing via fifo*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_entry, pfifo, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	TC_PRINT("main fifo put ---> ");
	tfifo_put(pfifo);
	irq_offload(tIsr_entry, pfifo);
	k_sem_take(&end_sema, K_FOREVER);
	k_sem_take(&end_sema, K_FOREVER);

	TC_PRINT("main fifo get\n");
	tfifo_get(pfifo);
	k_thread_abort(tid);
	TC_PRINT("\n");
}

/**
 * @addtogroup kernel_fifo_tests
 * @{
 */

/**
 * @brief Verify zephyr fifo continuous read write in loop
 *
 * @details
 * - Test Steps
 *   -# fifo put from main thread
 *   -# fifo read from isr
 *   -# fifo put from isr
 *   -# fifo get from spawn thread
 *   -# loop above steps for LOOPs times
 * - Expected Results
 *   -# fifo data pass correctly and stably across contexts
 *
 * @see k_fifo_init(), k_fifo_put(), k_fifo_get()
 */
void test_fifo_loop(void)
{
	k_fifo_init(&fifo);
	for (int i = 0; i < LOOPS; i++) {
		TC_PRINT("* Pass data by fifo in loop %d\n", i);
		tfifo_read_write(&fifo);
	}
}
/**
 * @}
 */
