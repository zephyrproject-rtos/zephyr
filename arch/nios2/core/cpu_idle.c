/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE
void arch_cpu_idle(void)
{
	/* Do nothing but unconditionally unlock interrupts and return to the
	 * caller. This CPU does not have any kind of power saving instruction.
	 */
	irq_unlock(NIOS2_STATUS_PIE_MSK);
}
#endif

#ifndef CONFIG_ARCH_HAS_CUSTOM_CPU_ATOMIC_IDLE
void arch_cpu_atomic_idle(unsigned int key)
{
	/* Do nothing but restore IRQ state. This CPU does not have any
	 * kind of power saving instruction.
	 */
	irq_unlock(key);
}
#endif
