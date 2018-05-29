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
#include <metal/irq.h>

extern void metal_generic_default_poll(void);

int metal_condition_wait(struct metal_condition *cv,
			 metal_mutex_t *m)
{
	metal_mutex_t *tmpm = 0;
	int v;
	unsigned int flags;

	/* Check if the mutex has been acquired */
	if (!cv || !m || !metal_mutex_is_acquired(m))
		return -EINVAL;

	if (!atomic_compare_exchange_strong(&cv->m, &tmpm, m)) {
		if (m != tmpm)
			return -EINVAL;
	}

	v = atomic_load(&cv->v);

	/* Release the mutex first. */
	metal_mutex_release(m);
	do {
		flags = metal_irq_save_disable();
		if (atomic_load(&cv->v) != v) {
			metal_irq_restore_enable(flags);
			break;
		}
		metal_generic_default_poll();
		metal_irq_restore_enable(flags);
	} while(1);
	/* Acquire the mutex again. */
	metal_mutex_acquire(m);
	return 0;
}
