/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <ksched.h>
#include <zephyr/wait_q.h>

extern struct k_spinlock z_pthread_spinlock;

int pthread_barrier_wait(pthread_barrier_t *b)
{
	k_spinlock_key_t key = k_spin_lock(&z_pthread_spinlock);
	int ret = 0;

	b->count++;

	if (b->count >= b->max) {
		b->count = 0;

		while (z_waitq_head(&b->wait_q)) {
			_ready_one_thread(&b->wait_q);
		}
		z_reschedule(&z_pthread_spinlock, key);
		ret = PTHREAD_BARRIER_SERIAL_THREAD;
	} else {
		(void) z_pend_curr(&z_pthread_spinlock, key, &b->wait_q, K_FOREVER);
	}

	return ret;
}
