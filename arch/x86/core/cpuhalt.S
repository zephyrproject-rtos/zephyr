/* cpuhalt.S - CPU power management code for IA-32 */

/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
DESCRIPTION
This module provides an implementation of the architecture-specific
nano_cpu_idle() primitive required by the nanokernel idle loop component.
It can be called within an implementation of _sys_power_save_idle(),
which is provided for the microkernel by the platform.

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

#include <arch/x86/asm.h>

	/* exports (external APIs) */

	GTEXT(nano_cpu_idle)
	GTEXT(nano_cpu_atomic_idle)

#if defined(CONFIG_BOOT_TIME_MEASUREMENT)
	GDATA(__idle_tsc)
#endif


/**
 *
 * @brief Power save idle routine for IA-32
 *
 * This function will be called by the nanokernel idle loop or possibly within
 * an implementation of _sys_power_save_idle in the microkernel when the
 * '_sys_power_save_flag' variable is non-zero.  The IA-32 'hlt' instruction
 * will be issued causing a low-power consumption sleep mode.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void nano_cpu_idle (void);
 */

SECTION_FUNC(TEXT, nano_cpu_idle)
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	call    _sys_k_event_logger_enter_sleep
#endif
#if defined(CONFIG_BOOT_TIME_MEASUREMENT)
	rdtsc			/* record idle timestamp */
	mov  %eax, __idle_tsc   /* ... low 32 bits  */
	mov  %edx, __idle_tsc+4 /* ... high 32 bits */
#endif
	sti			/* make sure interrupts are enabled */
	hlt			/* sleep */
	ret			/* return after processing ISR */


/**
 *
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * This function is utilized by the nanokernel object "wait" APIs for tasks,
 * e.g. nano_task_lifo_get_wait(), nano_task_sem_take_wait(),
 * nano_task_stack_pop_wait(), and nano_task_fifo_get_wait().
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
 * @return N/A
 *
 * C function prototype:
 *
 * void nano_cpu_atomic_idle (unsigned int imask);
 */

SECTION_FUNC(TEXT, nano_cpu_atomic_idle)
#ifdef CONFIG_INT_LATENCY_BENCHMARK
	call	_int_latency_stop
#endif
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	call    _sys_k_event_logger_enter_sleep
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

