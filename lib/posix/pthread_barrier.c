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
	int key = irq_lock(), ret, serial = 0;

	b->count++;

	if (b->count >= b->max) {
		b->count = 0;

		while (_waitq_head(&b->wait_q)) {
			_ready_one_thread(&b->wait_q);
		}
		serial = PTHREAD_BARRIER_SERIAL_THREAD;
		ret = _reschedule(key);
	} else {
		ret = _pend_current_thread(key, &b->wait_q, K_FOREVER);
	}

	return ret ? ret : serial;
}
