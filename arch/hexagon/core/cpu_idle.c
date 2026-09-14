/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/hexagon/arch.h>
#include <hexagon_vm.h>

/*
 * Simple idle -- wait for an interrupt, then re-enable guest interrupts.
 *
 * Called with guest interrupts disabled (IE=0).  Must return with
 * interrupts enabled (IE=1).
 *
 * H2's vmwait terminates on a pending interrupt even while the guest has
 * IE=0, so the wait itself races with nothing: the event is latched, and
 * the vmsetie below dispatches it through HVM_vm_check_interrupts on the
 * 0 -> 1 transition.
 */
void arch_cpu_idle(void)
{
	sys_trace_idle();
	hexagon_vm_wait();
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

	/*
	 * Wait while still locked, then restore IE.  vmwait terminates on a
	 * pending interrupt regardless of IE, and vmsetie enables IE and
	 * checks for pending interrupts inside the trap, so no wakeup can
	 * fall into the gap between the two.
	 */
	hexagon_vm_wait();
	hexagon_vm_setie(key);
}
