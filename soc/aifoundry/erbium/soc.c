/*
 * Copyright (c) 2026 AIFoundry
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/tracing/tracing.h>

/*
 * Erbium does not implement WFI. Wait for an enabled interrupt to become
 * pending with global interrupts masked, then restore the caller's state.
 * Keeping interrupts masked until after the check avoids losing a wakeup.
 */
static void erbium_idle(unsigned int key)
{
	(void)irq_lock();
#ifdef CONFIG_SYS_IDLE_HOOKS
	sys_trace_idle();
#endif
	while ((csr_read(mip) & csr_read(mie)) == 0UL) {
	}
#ifdef CONFIG_SYS_IDLE_HOOKS
	sys_trace_idle_exit();
#endif
	irq_unlock(key);
}

void arch_cpu_idle(void)
{
	erbium_idle(MSTATUS_IEN);
}

void arch_cpu_atomic_idle(unsigned int key)
{
	erbium_idle(key);
}
