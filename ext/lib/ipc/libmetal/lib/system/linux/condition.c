/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	generic/condition.c
 * @brief	Generic libmetal condition variable handling.
 */

#include <metal/condition.h>

int metal_condition_wait(struct metal_condition *cv,
				       metal_mutex_t *m)
{
	metal_mutex_t *tmpm = 0;
	int v = 0;

	/* Check if the mutex has been acquired */
	if (!cv || !m || !metal_mutex_is_acquired(m))
		return -EINVAL;

	if (!atomic_compare_exchange_strong(&cv->m, &tmpm, m)) {
		if (m != tmpm)
			return -EINVAL;
	}

	v = atomic_load(&cv->wakeups);
	atomic_fetch_add(&cv->waiters, 1);

	/* Release the mutex before sleeping. */
	metal_mutex_release(m);
	syscall(SYS_futex, &cv->wakeups, FUTEX_WAIT, v, NULL, NULL, 0);
	atomic_fetch_sub(&cv->waiters, 1);
	/* Acquire the mutex after it's waken up. */
	metal_mutex_acquire(m);

	return 0;
}
