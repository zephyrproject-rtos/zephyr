/* cpuhalt.s - VxMicro CPU power management code for IA-32 */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module provides an implementation of the architecture-specific
nano_cpu_idle() primitive required by the nanokernel idle loop component.
It can be called within an implementation of _sys_power_save_idle(),
which is provided for the microkernel by the BSP.

The module also provides an implementation of nano_cpu_atomic_idle(), which
atomically re-enables interrupts and enters low power mode.

INTERNAL
These implementations of nano_cpu_idle() and nano_cpu_atomic_idle() could be used
when operating as a Hypervisor guest.  More specifically, the Hypervisor
supports the execution of the 'hlt' instruction from a guest (results in a
VM exit), and more importantly, the Hypervisor will respect the
single instruction delay slot after the 'sti' instruction as required
by nano_cpu_atomic_idle().
*/

#define _ASMLANGUAGE

#include <nanokernel/x86/asm.h>

	/* exports (external APIs) */

	GTEXT(nano_cpu_idle)
	GTEXT(nano_cpu_atomic_idle)

#if defined(CONFIG_BOOT_TIME_MEASUREMENT)
	GDATA(__idle_tsc)
#endif

#ifndef CONFIG_NO_ISRS

/*******************************************************************************
*
* nano_cpu_idle - power save idle routine for IA-32
*
* This function will be called by the nanokernel idle loop or possibly within
* an implementation of _sys_power_save_idle in the microkernel when the
* '_sys_power_save_flag' variable is non-zero.  The IA-32 'hlt' instruction
* will be issued causing a low-power consumption sleep mode.
*
* RETURNS: N/A
*
* C function prototype:
*
* void nano_cpu_idle (void);
*/

SECTION_FUNC(TEXT, nano_cpu_idle)
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif
#if defined(CONFIG_BOOT_TIME_MEASUREMENT) 
	rdtsc			/* record idle timestamp */
	mov  %eax, __idle_tsc   /* ... low 32 bits  */
	mov  %edx, __idle_tsc+4 /* ... high 32 bits */
#endif
	sti			/* make sure interrupts are enabled */
	hlt			/* sleep */
	ret			/* return after processing ISR */


/*******************************************************************************
*
* nano_cpu_atomic_idle - atomically re-enable interrupts and enter low power mode
*
* This function is utilized by the nanokernel object "wait" APIs for task
* contexts, e.g. nano_task_lifo_get_wait(), nano_task_sem_take_wait(), nano_task_stack_pop_wait(),
* and nano_task_fifo_get_wait().
*
* INTERNAL
* The requirements for nano_cpu_atomic_idle() are as follows:
* 1) The enablement of interrupts and entering a low-power mode needs to be
*    atomic, i.e. there should be no period of time where interrupts are
*    enabled before the processor enters a low-power mode.  See the comments
*    in nano_task_lifo_get_wait(), for example, of the race condition that occurs
*    if this requirement is not met.
*
* 2) After waking up from the low-power mode, the interrupt lockout state
*    must be restored as indicated in the 'imask' input parameter.
*
* RETURNS: N/A
*
* C function prototype:
*
* void nano_cpu_atomic_idle (unsigned int imask);
*/

SECTION_FUNC(TEXT, nano_cpu_atomic_idle)
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif
	sti			/* make sure interrupts are enabled */

	/*
	 * The following statement appears in "Intel 64 and IA-32 Architectures
	 * Software Developer's Manual", regarding the 'sti' instruction:
	 *
	 *   "After the IF flag is set, the processor begins responding to
	 *    external, maskable interrupts after the next instruction is
	 *    executed."
	 *
	 * Thus the IA-32 implementation of nano_cpu_atomic_idle() will atomically
	 * re-enable interrupts and enter a low-power mode.
	 */

	hlt

	/* restore interrupt lockout state before returning to caller */

	testl $0x200, SP_ARG1(%esp)
	jnz skipIntDisable
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_start
#endif
	cli
BRANCH_LABEL(skipIntDisable)

	ret
	
#else

/*
 * When no interrupt support is configured, both nano_cpu_idle() and
 * nano_cpu_atomic_idle() are "no op" routines that simply return immediately
 * without entering low-power mode.
 */
 
SECTION_FUNC(TEXT, nano_cpu_idle)
SECTION_FUNC(TEXT, nano_cpu_atomic_idle)

	ret
	
#endif /* !CONFIG_NO_ISRS */

