/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <posix/pthread.h>
#include <ksched.h>
#include <wait_q.h>

int pthread_barrier_wait(pthread_barrier_t *b)
{
	unsigned int key = irq_lock();
	int ret = 0;

	b->count++;

	if (b->count >= b->max) {
		b->count = 0;

		while (z_waitq_head(&b->wait_q)) {
			_ready_one_thread(&b->wait_q);
		}
		z_reschedule_irqlock(key);
		ret = PTHREAD_BARRIER_SERIAL_THREAD;
	} else {
		(void) z_pend_curr_irqlock(key, &b->wait_q, K_FOREVER);
	}

	return ret;
}
