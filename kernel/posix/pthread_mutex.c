/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <pthread.h>
#include "ksched.h"
#include "wait_q.h"

int pthread_mutex_trylock(pthread_mutex_t *m)
{
	int key = irq_lock(), ret = EBUSY;

	if (m->sem->count) {
		m->sem->count = 0;
		ret = 0;
	}

	irq_unlock(key);

	return ret;
}
