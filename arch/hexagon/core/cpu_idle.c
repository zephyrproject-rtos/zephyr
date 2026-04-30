/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/arch.h>

/*
 * Ensure a timer interrupt is pending before entering idle.
 *
 * On QEMU, H2's vmwait path does not reliably wake the guest on
 * timer expiry alone.  Posting the timer interrupt manually before
 * re-enabling IE ensures the vmsetie trap's interrupt-check path
 * sees a pending interrupt and dispatches it immediately.
 */
static inline void hexagon_ensure_timer_tick(void)
{
	hexagon_vm_intop_post(CONFIG_HEXAGON_TIMER_IRQ, 0);
}

/*
 * Simple idle - post timer tick and re-enable guest interrupts.
 *
 * Called with guest interrupts disabled (IE=0).  Must return with
 * interrupts enabled (IE=1).
 *
 * Post the timer IRQ manually, then re-enable IE.  H2's vmsetie
 * path calls HVM_vm_check_interrupts when IE transitions 0 -> 1,
 * which dispatches the pending interrupt - the timer ISR runs
 * inside the vmsetie trap and vmrte returns here afterwards.
 */
void arch_cpu_idle(void)
{
	sys_trace_idle();
	hexagon_ensure_timer_tick();
	hexagon_vm_setie(VM_INT_ENABLE);
}

/*
 * Atomic idle -- re-enable interrupts and enter low-power state
 * atomically to avoid a race between the enable and the wait.
 *
 * Called with interrupts locked (key from arch_irq_lock).
 * Must re-enable interrupts before returning.
 */
void arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	hexagon_ensure_timer_tick();

	/*
	 * Post the timer tick while still locked, then restore IE.
	 * vmsetie atomically enables IE and checks for pending
	 * interrupts inside the trap -- so no window is lost.
	 */
	hexagon_vm_setie(key);
}

/* Power management states */
#ifdef CONFIG_PM
void arch_cpu_sleep(void)
{
	hexagon_vm_wait();
}

void arch_cpu_deep_sleep(void)
{
	hexagon_vm_wait();
}
#endif /* CONFIG_PM */
