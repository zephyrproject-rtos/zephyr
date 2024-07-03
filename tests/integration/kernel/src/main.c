/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <stdio.h>


/* COMMON DEFFINITIONS */
#define STACK_SIZE	500
#define LIST_LEN 8

static struct k_sem sema;
static struct k_thread tdata;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);


/* FIFO SCENARIO */
/* Thread A enters items into a fifo, starts Thread B and waits for a semaphore. */
/* Thread B extracts all items from the fifo and enters some items back into the fifo. */
/* Thread B gives the semaphore for Thread A to continue. */
/* Once the control is returned back to Thread A, it extracts all items from the fifo. */
/* Verify the data's correctness. */

static K_FIFO_DEFINE(fifo);
struct fifo_item_t {
	void *fifo_reserved;   /* 1st word reserved for use by FIFO */
	uint8_t value;
};
static struct fifo_item_t fifo_data[LIST_LEN];

static void thread_entry_fn_fifo(void *p1, void *p2, void *p3)
{
	struct fifo_item_t  *rx_data;
	uint32_t i;

	/* Get items from fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		rx_data = k_fifo_get((struct k_fifo *)p1, K_NO_WAIT);
		zassert_equal(rx_data->value, fifo_data[i].value);
	}

	/* Put items into fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		fifo_data[i].value *= i;
		k_fifo_put((struct k_fifo *)p1, &fifo_data[i]);
	}

	/* Give control back to Thread A */
	k_sem_give(&sema);
}


ZTEST(kernel, test_fifo_usage)
{
	struct fifo_item_t *rx_data;
	uint32_t i;

	/* Init binary semaphore */
	k_sem_init(&sema, 0, 1);

	/* Set and Put items into fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		fifo_data[i].value = i;
		k_fifo_put(&fifo, &fifo_data[i]);
	}

	/* Create the Thread B */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
					thread_entry_fn_fifo, &fifo, NULL, NULL,
					K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_NO_WAIT);

	/* Let the thread B run */
	k_sem_take(&sema, K_FOREVER);

	/* Get items from fifo */
	for (i = 0U; i < LIST_LEN; i++) {
		rx_data = k_fifo_get(&fifo, K_NO_WAIT);
		zassert_equal(rx_data->value, fifo_data[i].value);
	}

	/* Clear the spawn thread */
	k_thread_abort(tid);
}

/* LIFO SCENARIO */
/* Thread A enters items into a lifo, starts Thread B and waits for a semaphore. */
/* Thread B extracts all items from the lifo and enters some items back into the lifo. */
/* Thread B gives the semaphore for Thread A to continue. */
/* Once the control is returned back to Thread A, it extracts all items from the lifo. */
/* Verify the data's correctness. */

struct lifo_item_t {
	void *LIFO_reserved;   /* 1st word reserved for use by LIFO */
	uint8_t value;
};
static struct lifo_item_t lifo_data[LIST_LEN];
K_LIFO_DEFINE(lifo);


static void thread_entry_fn_lifo(void *p1, void *p2, void *p3)
{
	struct lifo_item_t  *rx_data;
	uint32_t i;

	/* Get items from lifo */
	for (i = 0U; i < LIST_LEN; i++) {
		rx_data = k_lifo_get((struct k_lifo *)p1, K_NO_WAIT);
		zassert_equal(rx_data->value, lifo_data[LIST_LEN-1-i].value);
	}

	/* Put items into lifo */
	for (i = 0U; i < LIST_LEN; i++) {
		lifo_data[i].value *= i;
		k_lifo_put((struct k_lifo *)p1, &lifo_data[i]);
	}

	/* Give control back to Thread A */
	k_sem_give(&sema);
}

ZTEST(kernel, test_lifo_usage)
{
	struct lifo_item_t *rx_data;
	uint32_t i;

	/* Init binary semaphore */
	k_sem_init(&sema, 0, 1);

	/* Set and Put items into lifo */
	for (i = 0U; i < LIST_LEN; i++) {
		lifo_data[i].value = i;
		k_lifo_put(&lifo, &lifo_data[i]);
	}

	/* Create the Thread B */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
					thread_entry_fn_lifo, &lifo, NULL, NULL,
					K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_NO_WAIT);

	/* Let the thread B run */
	k_sem_take(&sema, K_FOREVER);

	/* Get items from lifo */
	for (i = 0U; i < LIST_LEN; i++) {
		rx_data = k_lifo_get(&lifo, K_NO_WAIT);
		zassert_equal(rx_data->value, lifo_data[LIST_LEN-1-i].value);
	}

	/* Clear the spawn thread */
	k_thread_abort(tid);

}

/* STACK SCENARIO */
/* Thread A enters items into a stack, starts thread B and waits for a semaphore. */
/* Thread B extracts all items from the stack and enters some items back into the stack. */
/* Thread B gives the semaphore for Thread A to continue. */
/* Once the control is returned back to Thread A, it extracts all items from the stack. */
/* Verify the data's correctness. */


#define STACK_LEN	8
#define MAX_ITEMS	8

K_STACK_DEFINE(stack, STACK_LEN);
stack_data_t stack_data[MAX_ITEMS];


static void thread_entry_fn_stack(void *p1, void *p2, void *p3)
{
	stack_data_t *rx_data;
	stack_data_t data[MAX_ITEMS];

	/* fill data to compare */
	for (int i = 0; i < STACK_LEN; i++) {
		data[i] = i;
	}

	/* read data from a stack */
	for (int i = 0; i < STACK_LEN; i++) {
		k_stack_pop((struct k_stack *)p1, (stack_data_t *)&rx_data, K_NO_WAIT);
	}

	zassert_false(memcmp(rx_data, data, STACK_LEN),
		      "Push & Pop items does not match");

	/* Push data into a stack */
	for (int i = 0; i < STACK_LEN; i++) {
		stack_data[i] *= i;
		k_stack_push((struct k_stack *)p1, (stack_data_t)&stack_data[i]);
	}

	/* Give control back to Thread A */
	k_sem_give(&sema);
}


ZTEST(kernel, test_stack_usage)
{
	stack_data_t *rx_data;

	/* Init binary semaphore */
	k_sem_init(&sema, 0, 1);

	/* Push data into a stack */
	for (int i = 0; i < STACK_LEN; i++) {
		stack_data[i] = i;
		k_stack_push(&stack, (stack_data_t)&stack_data[i]);
	}

	/* Create the Thread B */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
					thread_entry_fn_stack, &stack, NULL, NULL,
					K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_NO_WAIT);

	/* Let the thread B run */
	k_sem_take(&sema, K_FOREVER);

	/* read data from a stack */
	for (int i = 0; i < STACK_LEN; i++) {
		k_stack_pop(&stack, (stack_data_t *)&rx_data, K_NO_WAIT);
	}


	/* Verify the correctness */
	zassert_false(memcmp(rx_data, stack_data, STACK_LEN),
		      "Push & Pop items does not match");

	/* Clear the spawn thread */
	k_thread_abort(tid);
}

/* MUTEX SCENARIO */
/* Initialize the mutex. */
/* Start the two Threads with the entry points. */
/* The entry points change variable and use lock and unlock mutex functions to */
/* synchronize the access to that variable. */
/* Join all threads. */
/* Verify the variable. */

#define NUMBER_OF_ITERATIONS 10000

static uint32_t mutex_data;
static struct k_thread tdata_2;
static K_THREAD_STACK_DEFINE(tstack_2, STACK_SIZE);

K_MUTEX_DEFINE(mutex);

static void thread_entry_fn_mutex(void *p1, void *p2, void *p3)
{
	uint32_t i;

	for (i = 0; i < NUMBER_OF_ITERATIONS; i++) {
		k_mutex_lock((struct k_mutex *)p1, K_FOREVER);
		mutex_data += 1;
		k_mutex_unlock((struct k_mutex *)p1);
	}
}

static void thread_entry_fn_mutex_2(void *p1, void *p2, void *p3)
{
	uint32_t i;

	for (i = 0; i < NUMBER_OF_ITERATIONS*2; i++) {
		k_mutex_lock((struct k_mutex *)p1, K_FOREVER);
		mutex_data -= 1;
		k_mutex_unlock((struct k_mutex *)p1);
	}
}

ZTEST(kernel, test_mutex_usage)
{

    /* Create the Thread A */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
					thread_entry_fn_mutex, &mutex, &mutex_data, NULL,
					K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_FOREVER);

	/* Create the Thread B */
	k_tid_t tid2 = k_thread_create(&tdata_2, tstack_2, STACK_SIZE,
					thread_entry_fn_mutex_2, &mutex, &mutex_data, NULL,
					K_PRIO_PREEMPT(0), K_INHERIT_PERMS, K_FOREVER);

	/* Start the Thread A */
	k_thread_start(tid);
	/* Start the Thread B */
	k_thread_start(tid2);
	/* Wait for end of Thread A */
	k_thread_join(&tdata, K_FOREVER);
	/* Wait for end of Thread B */
	k_thread_join(&tdata_2, K_FOREVER);
	/* Verify data after the end of the threads */
	zassert_equal(mutex_data, -10000);

	/* Clear the spawn threads */
	k_thread_abort(tid);
	k_thread_abort(tid2);
}


ZTEST_SUITE(kernel, NULL, NULL,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
