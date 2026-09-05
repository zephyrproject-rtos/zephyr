/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/mpipe/mpipe_thread.h>

K_THREAD_STACK_ARRAY_DEFINE(thread_stack, CONFIG_MPIPE_THREADS_NUM, CONFIG_MPIPE_THREAD_STACK_SIZE);

static bool mpipe_thread_stack_pool[CONFIG_MPIPE_THREADS_NUM];

k_tid_t mpipe_thread_create(struct mpipe_thread *thread, k_thread_entry_t func, void *p1, void *p2,
			    void *p3, int priority, k_timeout_t delay)
{
	int id;

	/* Find the 1st available slot in the thread pool */
	for (id = 0; id < CONFIG_MPIPE_THREADS_NUM; id++) {
		if (mpipe_thread_stack_pool[id] == false) {
			break;
		}
	}

	if (id == CONFIG_MPIPE_THREADS_NUM) {
		return NULL;
	}

	thread->stack_id = id;
	atomic_set(&thread->state, MPIPE_THREAD_PAUSED);
	mpipe_thread_stack_pool[id] = true;

	/* Semaphore starts at 0 to be able to block the thread with mpipe_thread_wait() */
	k_sem_init(&thread->sem, 0, 1);

	return k_thread_create(&thread->thread, thread_stack[thread->stack_id],
			       K_THREAD_STACK_SIZEOF(thread_stack[thread->stack_id]), func, p1, p2,
			       p3, priority, 0, delay);
}

int mpipe_thread_wait(struct mpipe_thread *thread)
{
	__ASSERT_NO_MSG(thread != NULL);

	for (;;) {
		int state = atomic_get(&thread->state);

		if (state == MPIPE_THREAD_RUNNING) {
			return 0;
		}

		if (state == MPIPE_THREAD_TERMINATED) {
			return -ECANCELED;
		}

		/* Block until someone resumes or terminates us. */
		k_sem_take(&thread->sem, K_FOREVER);
	}
}

void mpipe_thread_resume(struct mpipe_thread *thread)
{
	__ASSERT_NO_MSG(thread != NULL);

	/* Resume must not override a concurrent join(). Only transition PAUSED -> RUNNING */
	for (;;) {
		int state = atomic_get(&thread->state);

		if (state == MPIPE_THREAD_TERMINATED) {
			return;
		}

		if (state == MPIPE_THREAD_RUNNING ||
		    atomic_cas(&thread->state, MPIPE_THREAD_PAUSED, MPIPE_THREAD_RUNNING)) {
			break;
		}
	}

	/* Wake from initial K_FOREVER sleep (no-op if already awake) */
	k_wakeup(&thread->thread);
	/* Unblock from wait() if blocked on the semaphore */
	k_sem_give(&thread->sem);
}

void mpipe_thread_pause(struct mpipe_thread *thread)
{
	__ASSERT_NO_MSG(thread != NULL);

	/* Pause must not override a concurrent join(). Only transition RUNNING -> PAUSED */
	for (;;) {
		int state = atomic_get(&thread->state);

		if (state == MPIPE_THREAD_TERMINATED || state == MPIPE_THREAD_PAUSED ||
		    atomic_cas(&thread->state, MPIPE_THREAD_RUNNING, MPIPE_THREAD_PAUSED)) {
			return;
		}
	}
}

int mpipe_thread_join(struct mpipe_thread *thread, k_timeout_t timeout)
{
	int ret;

	__ASSERT_NO_MSG(thread != NULL);

	/* Signal the thread to exit */
	atomic_set(&thread->state, MPIPE_THREAD_TERMINATED);

	/* Wake from initial sleep in case the thread was never resumed */
	k_wakeup(&thread->thread);

	/* Unblock from wait() in case it is blocked on the semaphore */
	k_sem_give(&thread->sem);

	ret = k_thread_join(&thread->thread, timeout);
	if (ret == 0) {
		mpipe_thread_stack_pool[thread->stack_id] = false;
	}

	return ret;
}
