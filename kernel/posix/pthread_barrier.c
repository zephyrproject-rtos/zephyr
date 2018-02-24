/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <pthread.h>
#include "ksched.h"
#include "wait_q.h"
#include <kswap.h>

void ready_one_thread(_wait_q_t *wq);

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
