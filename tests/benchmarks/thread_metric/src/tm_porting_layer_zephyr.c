/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 * Copyright (c) 2024 Intel Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** Thread-Metric Component                                               */
/**                                                                       */
/**   Porting Layer (Must be completed with RTOS specifics)               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/* Include necessary files.  */

#include "tm_api.h"

#include <zephyr/kernel.h>

#define TM_TEST_NUM_THREADS        10
#define TM_TEST_STACK_SIZE         1024
#define TM_TEST_NUM_SEMAPHORES     4
#define TM_TEST_NUM_MESSAGE_QUEUES 4
#define TM_TEST_NUM_SLABS          4

#if (CONFIG_MP_MAX_NUM_CPUS > 1)
#error "*** Tests are only designed for single processor systems! ***"
#endif

static struct k_thread test_thread[TM_TEST_NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(test_stack, TM_TEST_NUM_THREADS, TM_TEST_STACK_SIZE);

static struct k_sem test_sem[TM_TEST_NUM_SEMAPHORES];

static struct k_msgq test_msgq[TM_TEST_NUM_MESSAGE_QUEUES];
static char test_msgq_buffer[TM_TEST_NUM_MESSAGE_QUEUES][8][16];

static struct k_mem_slab test_slab[TM_TEST_NUM_SLABS];
static char __aligned(4) test_slab_buffer[TM_TEST_NUM_SLABS][8 * 128];

/*
 * This function called from main performs basic RTOS initialization,
 * calls the test initialization function, and then starts the RTOS function.
 */
void tm_initialize(void (*test_initialization_function)(void))
{
	test_initialization_function();
}

/*
 * This function takes a thread ID and priority and attempts to create the
 * file in the underlying RTOS.  Valid priorities range from 1 through 31,
 * where 1 is the highest priority and 31 is the lowest. If successful,
 * the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_thread_create(int thread_id, int priority, void (*entry_function)(void *, void *, void *))
{
	k_tid_t tid;

	tid = k_thread_create(&test_thread[thread_id], test_stack[thread_id],
			      TM_TEST_STACK_SIZE, entry_function,
			      NULL, NULL, NULL, priority, 0, K_FOREVER);

	return (tid == &test_thread[thread_id]) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function resumes the specified thread.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_thread_resume(int thread_id)
{
	if (test_thread[thread_id].base.thread_state & _THREAD_PRESTART) {
		k_thread_start(&test_thread[thread_id]);
	} else {
		k_thread_resume(&test_thread[thread_id]);
	}

	return TM_SUCCESS;
}

/*
 * This function suspends the specified thread.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_thread_suspend(int thread_id)
{
	k_thread_suspend(&test_thread[thread_id]);

	return TM_SUCCESS;
}

/*
 * This function relinquishes to other ready threads at the same
 * priority.
 */
void tm_thread_relinquish(void)
{
	k_yield();
}

/*
 * This function suspends the specified thread for the specified number
 * of seconds.
 */
void tm_thread_sleep(int seconds)
{
	k_sleep(K_SECONDS(seconds));
}

/*
 * This function creates the specified queue.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_queue_create(int queue_id)
{
	k_msgq_init(&test_msgq[queue_id], &test_msgq_buffer[queue_id][0][0], 16, 8);

	return TM_SUCCESS;
}

/*
 * This function sends a 16-byte message to the specified queue.  If successful,
 * the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
	return k_msgq_put(&test_msgq[queue_id], message_ptr, K_FOREVER);
}

/*
 * This function receives a 16-byte message from the specified queue.  If successful,
 * the function should return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
	return k_msgq_get(&test_msgq[queue_id], message_ptr, K_FOREVER);
}

/*
 * This function creates the specified semaphore.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_semaphore_create(int semaphore_id)
{
	/* Create an available semaphore with max count of 1 */
	return k_sem_init(&test_sem[semaphore_id], 1, 1);
}

/*
 * This function gets the specified semaphore.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_semaphore_get(int semaphore_id)
{
	return k_sem_take(&test_sem[semaphore_id], K_NO_WAIT);
}

/*
 * This function puts the specified semaphore.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_semaphore_put(int semaphore_id)
{
	k_sem_give(&test_sem[semaphore_id]);
	return TM_SUCCESS;
}

/* This function is defined by the benchmark. */
extern void tm_interrupt_handler(const void *);

void tm_cause_interrupt(void)
{
	irq_offload(tm_interrupt_handler, NULL);
}

/*
 * This function creates the specified memory pool that can support one or more
 * allocations of 128 bytes.  If successful, the function should
 * return TM_SUCCESS. Otherwise, TM_ERROR should be returned.
 */
int tm_memory_pool_create(int pool_id)
{
	int status;

	status = k_mem_slab_init(&test_slab[pool_id], &test_slab_buffer[pool_id][0], 128, 8);

	return (status == 0) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function allocates a 128 byte block from the specified memory pool.
 * If successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR
 * should be returned.
 */
int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
	int status;

	status = k_mem_slab_alloc(&test_slab[pool_id], (void **)memory_ptr, K_NO_WAIT);

	return (status == 0) ? TM_SUCCESS : TM_ERROR;
}

/*
 * This function releases a previously allocated 128 byte block from the specified
 * memory pool. If successful, the function should return TM_SUCCESS. Otherwise, TM_ERROR
 * should be returned.
 */
int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
	k_mem_slab_free(&test_slab[pool_id], (void *)memory_ptr);

	return TM_SUCCESS;
}
