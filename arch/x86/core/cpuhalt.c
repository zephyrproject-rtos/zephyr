/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <debug/tracing.h>
#include <arch/cpu.h>

void z_arch_cpu_idle(void)
{
	sys_trace_idle();
	__asm__ volatile (
	    "sti\n\t"
	    "hlt\n\t");
}

void z_arch_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();

	__asm__ volatile (
	    "sti\n\t"
	    /*
	     * The following statement appears in "Intel 64 and IA-32
	     * Architectures Software Developer's Manual", regarding the 'sti'
	     * instruction:
	     *
	     * "After the IF flag is set, the processor begins responding to
	     *    external, maskable interrupts after the next instruction is
	     *    executed."
	     *
	     * Thus the IA-32 implementation of z_arch_cpu_atomic_idle() will
	     * atomically re-enable interrupts and enter a low-power mode.
	     */
	    "hlt\n\t");

	/* restore interrupt lockout state before returning to caller */
	if ((key & 0x200U) == 0U) {
		__asm__ volatile("cli");
	}
}
