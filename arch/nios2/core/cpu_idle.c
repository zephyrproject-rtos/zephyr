/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_structs.h>

void z_arch_cpu_idle(void)
{
	/* Do nothing but unconditionally unlock interrupts and return to the
	 * caller. This CPU does not have any kind of power saving instruction.
	 */
	irq_unlock(NIOS2_STATUS_PIE_MSK);
}

void z_arch_cpu_atomic_idle(unsigned int key)
{
	/* Do nothing but restore IRQ state. This CPU does not have any
	 * kind of power saving instruction.
	 */
	irq_unlock(key);
}
