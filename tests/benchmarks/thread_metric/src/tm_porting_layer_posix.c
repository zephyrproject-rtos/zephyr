/***************************************************************************
 * Copyright The Zephyr Project Contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/*
 * Thread-Metric porting layer for Zephyr's POSIX API implementation.
 */

#include "tm_api.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TM_TEST_NUM_THREADS        5
#define TM_TEST_NUM_SEMAPHORES     4
#define TM_TEST_NUM_MESSAGE_QUEUES 4
#define TM_TEST_NUM_MEMORY_POOLS   4
#define TM_TEST_MESSAGE_SIZE       16
#define TM_TEST_QUEUE_DEPTH        8
#define TM_TEST_MEMORY_SIZE        128

struct tm_posix_thread {
	pthread_t thread;
	pthread_attr_t attr;
	sem_t suspend_sem;
	void (*entry_function)(void *p1, void *p2, void *p3);
};

static struct tm_posix_thread test_thread[TM_TEST_NUM_THREADS];
static sem_t test_sem[TM_TEST_NUM_SEMAPHORES];
static mqd_t test_queue[TM_TEST_NUM_MESSAGE_QUEUES];

static int tm_sem_wait(sem_t *sem)
{
	int status;

	do {
		status = sem_wait(sem);
	} while ((status < 0) && (errno == EINTR));

	return (status == 0) ? TM_SUCCESS : TM_ERROR;
}

static void *tm_thread_entry(void *arg)
{
	struct tm_posix_thread *thread = arg;

	if (tm_sem_wait(&thread->suspend_sem) != TM_SUCCESS) {
		return NULL;
	}

	thread->entry_function(NULL, NULL, NULL);

	return NULL;
}

void tm_initialize(void (*test_initialization_function)(void))
{
	test_initialization_function();
}

int tm_thread_create(int thread_id, int priority,
		     void (*entry_function)(void *p1, void *p2, void *p3))
{
	pthread_attr_t *attr;
	struct sched_param sched_param;
	int priority_max;
	int priority_min;
	int status;

	if ((thread_id < 0) || (thread_id >= TM_TEST_NUM_THREADS)) {
		return TM_ERROR;
	}

	attr = &test_thread[thread_id].attr;
	status = sem_init(&test_thread[thread_id].suspend_sem, 0, 0);
	if (status != 0) {
		return TM_ERROR;
	}

	test_thread[thread_id].entry_function = entry_function;

	status = pthread_attr_init(attr);
	if (status != 0) {
		(void)sem_destroy(&test_thread[thread_id].suspend_sem);
		return TM_ERROR;
	}

	status = pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
	status = status == 0 ? pthread_attr_setschedpolicy(attr, SCHED_RR) : status;
	priority_max = sched_get_priority_max(SCHED_RR);
	priority_min = sched_get_priority_min(SCHED_RR);
	if ((priority_max < 0) || (priority_min < 0) || (priority < 0) ||
	    (priority > (priority_max - priority_min))) {
		status = EINVAL;
	}
	sched_param.sched_priority = priority_max - priority;
	status = status == 0 ? pthread_attr_setschedparam(attr, &sched_param) : status;
	status = status == 0 ? pthread_create(&test_thread[thread_id].thread, attr, tm_thread_entry,
					      &test_thread[thread_id])
			     : status;

	if (status != 0) {
		(void)pthread_attr_destroy(attr);
		(void)sem_destroy(&test_thread[thread_id].suspend_sem);
	}
	/* Keep successful attributes alive because they own the persistent thread's stack. */

	return (status == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_resume(int thread_id)
{
	if ((thread_id < 0) || (thread_id >= TM_TEST_NUM_THREADS)) {
		return TM_ERROR;
	}

	return (sem_post(&test_thread[thread_id].suspend_sem) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_thread_suspend(int thread_id)
{
	if ((thread_id < 0) || (thread_id >= TM_TEST_NUM_THREADS)) {
		return TM_ERROR;
	}

	return tm_sem_wait(&test_thread[thread_id].suspend_sem);
}

void tm_thread_relinquish(void)
{
	(void)sched_yield();
}

void tm_thread_sleep(int seconds)
{
	(void)sleep(seconds);
}

int tm_queue_create(int queue_id)
{
	char queue_name[12];
	struct mq_attr attr = {
		.mq_maxmsg = TM_TEST_QUEUE_DEPTH,
		.mq_msgsize = TM_TEST_MESSAGE_SIZE,
	};

	if ((queue_id < 0) || (queue_id >= TM_TEST_NUM_MESSAGE_QUEUES)) {
		return TM_ERROR;
	}

	(void)snprintf(queue_name, sizeof(queue_name), "/tmq_%d", queue_id);
	test_queue[queue_id] = mq_open(queue_name, O_CREAT | O_RDWR, 0600, &attr);

	return (test_queue[queue_id] != (mqd_t)-1) ? TM_SUCCESS : TM_ERROR;
}

int tm_queue_send(int queue_id, unsigned long *message_ptr)
{
	if ((queue_id < 0) || (queue_id >= TM_TEST_NUM_MESSAGE_QUEUES)) {
		return TM_ERROR;
	}

	return (mq_send(test_queue[queue_id], (const char *)message_ptr, TM_TEST_MESSAGE_SIZE, 0) ==
		0)
		       ? TM_SUCCESS
		       : TM_ERROR;
}

int tm_queue_receive(int queue_id, unsigned long *message_ptr)
{
	ssize_t received;

	if ((queue_id < 0) || (queue_id >= TM_TEST_NUM_MESSAGE_QUEUES)) {
		return TM_ERROR;
	}

	received =
		mq_receive(test_queue[queue_id], (char *)message_ptr, TM_TEST_MESSAGE_SIZE, NULL);

	return (received == TM_TEST_MESSAGE_SIZE) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_create(int semaphore_id)
{
	if ((semaphore_id < 0) || (semaphore_id >= TM_TEST_NUM_SEMAPHORES)) {
		return TM_ERROR;
	}

	return (sem_init(&test_sem[semaphore_id], 0, 1) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_semaphore_get(int semaphore_id)
{
	if ((semaphore_id < 0) || (semaphore_id >= TM_TEST_NUM_SEMAPHORES)) {
		return TM_ERROR;
	}

	return tm_sem_wait(&test_sem[semaphore_id]);
}

int tm_semaphore_put(int semaphore_id)
{
	if ((semaphore_id < 0) || (semaphore_id >= TM_TEST_NUM_SEMAPHORES)) {
		return TM_ERROR;
	}

	return (sem_post(&test_sem[semaphore_id]) == 0) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_create(int pool_id)
{
	return ((pool_id >= 0) && (pool_id < TM_TEST_NUM_MEMORY_POOLS)) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_allocate(int pool_id, unsigned char **memory_ptr)
{
	if ((pool_id < 0) || (pool_id >= TM_TEST_NUM_MEMORY_POOLS) || (memory_ptr == NULL)) {
		return TM_ERROR;
	}

	*memory_ptr = malloc(TM_TEST_MEMORY_SIZE);

	return (*memory_ptr != NULL) ? TM_SUCCESS : TM_ERROR;
}

int tm_memory_pool_deallocate(int pool_id, unsigned char *memory_ptr)
{
	if ((pool_id < 0) || (pool_id >= TM_TEST_NUM_MEMORY_POOLS) || (memory_ptr == NULL)) {
		return TM_ERROR;
	}

	free(memory_ptr);

	return TM_SUCCESS;
}
