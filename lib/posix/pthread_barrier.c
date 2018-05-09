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
	int key = irq_lock();

	b->count++;

	if (b->count >= b->max) {
		b->count = 0;

		while (!sys_dlist_is_empty(&b->wait_q)) {
			_ready_one_thread(&b->wait_q);
		}
		return _reschedule(key);
	} else {
		return _pend_current_thread(key, &b->wait_q, K_FOREVER);
	}
}
