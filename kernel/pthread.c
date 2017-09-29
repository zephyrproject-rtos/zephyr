/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <pthread.h>
#include "include/ksched.h"
#include "include/wait_q.h"

static void ready_one_thread(_wait_q_t *wq)
{
	struct k_thread *th = _unpend_first_thread(wq);

	if (th) {
		_abort_thread_timeout(th);
		_ready_thread(th);
	}
}

static int cond_wait(pthread_cond_t *cv, pthread_mutex_t *mut, int timeout)
{
	__ASSERT(mut->sem->count == 0, "");

	int ret, key = irq_lock();

	mut->sem->count = 1;
	ready_one_thread(&mut->sem->wait_q);
	_pend_current_thread(&cv->wait_q, timeout);

	ret = _Swap(key);

	/* FIXME: this extra lock (and the potential context switch it
	 * can cause) could be optimized out.  At the point of the
	 * signal/broadcast, it's possible to detect whether or not we
	 * will be swapping back to this particular thread and lock it
	 * (i.e. leave the lock variable unchanged) on our behalf.
	 * But that requires putting scheduler intelligence into this
	 * higher level abstraction and is probably not worth it.
	 */
	pthread_mutex_lock(mut);

	return ret == -EAGAIN ? -ETIMEDOUT : ret;
}

/* This implements a "fair" scheduling policy: at the end of a POSIX
 * thread call that might result in a change of the current maximum
 * priority thread, we always check and context switch if needed.
 * Note that there is significant dispute in the community over the
 * "right" way to do this and different systems do it differently by
 * default.  Zephyr is an RTOS, so we choose latency over
 * throughput.  See here for a good discussion of the broad issue:
 *
 * https://blog.mozilla.org/nfroyd/2017/03/29/on-mutex-performance-part-1/
 */
static void swap_or_unlock(int key)
{
	/* API madness: use __ not _ here.  The latter checks for our
	 * preemption state, but we want to do a switch here even if
	 * we can be preempted.
	 */
	if (!_is_in_isr() && __must_switch_threads()) {
		_Swap(key);
	} else {
		irq_unlock(key);
	}
}

int pthread_cond_signal(pthread_cond_t *cv)
{
	int key = irq_lock();

	ready_one_thread(&cv->wait_q);
	swap_or_unlock(key);

	return 0;
}

int pthread_cond_broadcast(pthread_cond_t *cv)
{
	int key = irq_lock();

	while (!sys_dlist_is_empty(&cv->wait_q)) {
		ready_one_thread(&cv->wait_q);
	}

	swap_or_unlock(key);

	return 0;
}

int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mut)
{
	return cond_wait(cv, mut, K_FOREVER);
}

int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *mut,
			   const struct timespec *to)
{
	return cond_wait(cv, mut, _ts_to_ms(to));
}

int pthread_mutex_trylock(pthread_mutex_t *m)
{
	int key = irq_lock(), ret = -EBUSY;

	if (m->sem->count) {
		m->sem->count = 0;
		ret = 0;
	}

	irq_unlock(key);

	return ret;
}

int pthread_barrier_wait(pthread_barrier_t *b)
{
	int key = irq_lock();

	b->count++;

	if (b->count >= b->max) {
		b->count = 0;

		while (!sys_dlist_is_empty(&b->wait_q)) {
			ready_one_thread(&b->wait_q);
		}

		if (!__must_switch_threads()) {
			irq_unlock(key);
			return 0;
		}
	} else {
		_pend_current_thread(&b->wait_q, K_FOREVER);
	}

	return _Swap(key);
}
