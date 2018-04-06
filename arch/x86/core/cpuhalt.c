/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file  CPU power management code for IA-32
 *
 * This module provides an implementation of the architecture-specific
 * k_cpu_idle() primitive required by the kernel idle loop component.
 * It can be called within an implementation of _sys_power_save_idle(),
 * which is provided for the kernel by the platform.
 *
 * The module also provides an implementation of k_cpu_atomic_idle(), which
 * atomically re-enables interrupts and enters low power mode.
 *
 * INTERNAL
 * These implementations of k_cpu_idle() and k_cpu_atomic_idle() could be
 * used when operating as a Hypervisor guest.  More specifically, the Hypervisor
 * supports the execution of the 'hlt' instruction from a guest (results in a
 * VM exit), and more importantly, the Hypervisor will respect the
 * single instruction delay slot after the 'sti' instruction as required
 * by k_cpu_atomic_idle().
 */

#include <zephyr.h>
#include <tracing.h>
#include <arch/cpu.h>

#ifdef CONFIG_BOOT_TIME_MEASUREMENT
extern u64_t __idle_time_stamp;  /* timestamp when CPU went idle */
#endif

/**
 *
 * @brief Power save idle routine for IA-32
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of _sys_power_save_idle in the kernel when the
 * '_sys_power_save_flag' variable is non-zero.  The IA-32 'hlt' instruction
 * will be issued causing a low-power consumption sleep mode.
 *
 * @return N/A
 */
void k_cpu_idle(void)
{
	_int_latency_stop();
	z_sys_trace_idle();
#if defined(CONFIG_BOOT_TIME_MEASUREMENT)
	__idle_time_stamp = (u64_t)k_cycle_get_32();
#endif

	__asm__ volatile (
	    "sti\n\t"
	    "hlt\n\t");
}

/**
 *
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * INTERNAL
 * The requirements for k_cpu_atomic_idle() are as follows:
 * 1) The enablement of interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that
 *    occurs if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'imask' input parameter.
 *
 * @return N/A
 */

void k_cpu_atomic_idle(unsigned int imask)
{
	_int_latency_stop();
	z_sys_trace_idle();

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
	     * Thus the IA-32 implementation of k_cpu_atomic_idle() will
	     * atomically re-enable interrupts and enter a low-power mode.
	     */
	    "hlt\n\t");

	/* restore interrupt lockout state before returning to caller */
	if (!(imask & 0x200)) {
		_int_latency_start();
		__asm__ volatile("cli");
	}
}
