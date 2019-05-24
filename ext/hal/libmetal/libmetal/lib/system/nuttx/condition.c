/*
 * Copyright (c) 2018, Pinecone Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	nuttx/condition.c
 * @brief	NuttX libmetal condition variable handling.
 */

#include <metal/condition.h>
#include <metal/irq.h>

int metal_condition_wait(struct metal_condition *cv,
			 metal_mutex_t *m)
{
	unsigned int flags;

	/* Check if the mutex has been acquired */
	if (!cv || !m || !metal_mutex_is_acquired(m))
		return -EINVAL;

	flags = metal_irq_save_disable();
	/* Release the mutex first. */
	metal_mutex_release(m);
	nxsem_wait_uninterruptible(&cv->cond.sem);
	metal_irq_restore_enable(flags);
	/* Acquire the mutex again. */
	metal_mutex_acquire(m);
	return 0;
}
