/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <pthread.h>
#include "ksched.h"
#include "wait_q.h"

void ready_one_thread(_wait_q_t *wq)
{
	struct k_thread *th = _unpend_first_thread(wq);

	if (th) {
		_abort_thread_timeout(th);
		_ready_thread(th);
	}
}
