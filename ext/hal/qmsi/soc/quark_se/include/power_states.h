/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __POWER_STATES_H__
#define __POWER_STATES_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * SoC Power mode control for Quark SE Microcontrollers.
 *
 * Available SoC states are:
 *     - Low Power Sensing Standby (LPSS)
 *     - Sleep
 *
 * LPSS can only be enabled from the Sensor core,
 * refer to @ref qm_ss_power_soc_lpss_enable for further details.
 *
 * @defgroup groupSoCPower Quark SE SoC Power states
 * @{
 */

/**
 * Enter SoC sleep state.
 *
 * Put the SoC into sleep state until next SoC wake event.
 *
 * - Core well is turned off
 * - Always on well is on
 * - Hybrid Clock is off
 * - RTC Clock is on
 *
 * Possible SoC wake events are:
 * 	- Low Power Comparator Interrupt
 * 	- AON GPIO Interrupt
 * 	- AON Timer Interrupt
 * 	- RTC Interrupt
 */
void qm_power_soc_sleep(void);

/**
 * Enter SoC deep sleep state.
 *
 * Put the SoC into deep sleep state until next SoC wake event.
 *
 * - Core well is turned off
 * - Always on well is on
 * - Hybrid Clock is off
 * - RTC Clock is on
 *
 * Possible SoC wake events are:
 * 	- Low Power Comparator Interrupt
 * 	- AON GPIO Interrupt
 * 	- AON Timer Interrupt
 * 	- RTC Interrupt
 *
 * This function puts 1P8V regulators and 3P3V into Linear Mode.
 */
void qm_power_soc_deep_sleep(void);

#if (ENABLE_RESTORE_CONTEXT) && (!QM_SENSOR)
/**
 * Enter SoC sleep state and restore after wake up.
 *
 * Put the SoC into sleep state until next SoC wake event
 * and continue execution after wake up where the application stopped.
 *
 * If the library is built with ENABLE_RESTORE_CONTEXT=1, then this function
 * will use the common RAM __x86_restore_info[0] to save the necessary context
 * to bring back the CPU to the point where this function was called.
 * This means that applications should refrain from using them.
 *
 * This function calls qm_x86_save_context and qm_x86_restore_context
 * in order to restore execution where it stopped.
 * All power management transitions are done by qm_power_soc_sleep().
 */
void qm_power_soc_sleep_restore(void);

/**
 * Enter SoC deep sleep state and restore after wake up.
 *
 * Put the SoC into deep sleep state until next SoC wake event
 * and continue execution after wake up where the application stopped.
 *
 * If the library is built with ENABLE_RESTORE_CONTEXT=1, then this function
 * will use the common RAM __x86_restore_info[0] to save the necessary context
 * to bring back the CPU to the point where this function was called.
 * This means that applications should refrain from using them.
 *
 * This function calls qm_x86_save_context and qm_x86_restore_context
 * in order to restore execution where it stopped.
 * All power management transitions are done by qm_power_soc_deep_sleep().
 */
void qm_power_soc_deep_sleep_restore(void);

/**
 * Save context, enter x86 C2 power save state and restore after wake up.
 *
 * This routine is same as qm_power_soc_sleep_restore(), just instead of
 * going to sleep it will go to C2 power save state.
 * Note: this function has a while(1) which will spin until we enter
 * (and exit) sleep while the power state change will be managed by
 * the other core.
 */
void qm_power_sleep_wait(void);

/**
 * Enable the x86 startup restore flag, see GPS0 #define in qm_soc_regs.h
 */
void qm_power_soc_set_x86_restore_flag(void);

#endif /* ENABLE_RESTORE_CONTEXT */

/**
 * @}
 */

#if (!QM_SENSOR)
/**
 * Host Power mode control for Quark SE Microcontrollers.<BR>
 * These functions cannot be called from the Sensor Subsystem.
 *
 * @defgroup groupSEPower Quark SE Host Power states
 * @{
 */

/**
 * Enter Host C1 state.
 *
 * Put the Host into C1.<BR>
 * Processor Clock is gated in this state.<BR>
 * Nothing is turned off in this state.
 *
 * This function can be called with interrupts disabled.
 * Interrupts will be enabled before triggering the transition.
 *
 * A wake event causes the Host to transition to C0.<BR>
 * A wake event is a host interrupt.
 */
void qm_power_cpu_c1(void);

/**
 * Enter Host C2 state or SoC LPSS state.
 *
 * Put the Host into C2.
 * Processor Clock is gated in this state.
 * All rails are supplied.
 *
 * This enables entry in LPSS if:
 *  - Sensor Subsystem is in SS2.
 *  - LPSS entry is enabled.
 *
 * If C2 is entered:
 *  - A wake event causes the Host to transition to C0.
 *  - A wake event is a host interrupt.
 *
 * If LPSS is entered:
 *  - LPSS wake events applies.
 *  - If the Sensor Subsystem wakes the SoC from LPSS, Host is back in C2.
 */
void qm_power_cpu_c2(void);

/**
 * Enter Host C2LP state or SoC LPSS state.
 *
 * Put the Host into C2LP.
 * Processor Complex Clock is gated in this state.
 * All rails are supplied.
 *
 * This enables entry in LPSS if:
 *  - Sensor Subsystem is in SS2.
 *  - LPSS is allowed.
 *
 * If C2LP is entered:
 *  - A wake event causes the Host to transition to C0.
 *  - A wake event is a Host interrupt.
 *
 * If LPSS is entered:
 *  - LPSS wake events apply if LPSS is entered.
 *  - If the Sensor Subsystem wakes the SoC from LPSS,
 *    Host transitions back to C2LP.
 */
void qm_power_cpu_c2lp(void);
#endif

#if (ENABLE_RESTORE_CONTEXT) && (!QM_SENSOR) && (!UNIT_TEST)
/**
 * Save resume vector.
 *
 * Saves the resume vector in the common RAM __x86_restore_info[0] location.
 * The bootloader will jump to the resume vector once a wake up event
 * is triggered.
 */
#define qm_x86_set_resume_vector(_restore_label, shared_mem)                   \
	__asm__ __volatile__("movl $" #_restore_label ", %[trap]\n\t"          \
			     : /* Output operands. */                          \
			     : /* Input operands. */                           \
			     [trap] "m"(shared_mem)                            \
			     : /* Clobbered registers list. */                 \
			     )

/* Save execution context.
 *
 * This routine saves 'idtr', EFLAGS and general purpose registers onto the
 * stack.
 *
 * The bootloader will set the 'gdt' before calling into the 'restore_trap'
 * function, so we don't need to save it here.
 */
#define qm_x86_save_context(stack_pointer)                                     \
	__asm__ __volatile__("sub $8, %%esp\n\t"                               \
			     "sidt (%%esp)\n\t"                                \
			     "lea %[stackpointer], %%eax\n\t"                  \
			     "pushfl\n\t"                                      \
			     "pushal\n\t"                                      \
			     "movl %%dr0, %%edx\n\t"                           \
			     "pushl %%edx\n\t"                                 \
			     "movl %%dr1, %%edx\n\t"                           \
			     "pushl %%edx\n\t"                                 \
			     "movl %%dr2, %%edx\n\t"                           \
			     "pushl %%edx\n\t"                                 \
			     "movl %%dr3, %%edx\n\t"                           \
			     "pushl %%edx\n\t"                                 \
			     "movl %%dr6, %%edx\n\t"                           \
			     "pushl %%edx\n\t"                                 \
			     "movl %%dr7, %%edx\n\t"                           \
			     "pushl %%edx\n\t"                                 \
			     "movl %%esp, (%%eax)\n\t"                         \
			     : /* Output operands. */                          \
			     : /* Input operands. */                           \
			     [stackpointer] "m"(stack_pointer)                 \
			     : /* Clobbered registers list. */                 \
			     "eax", "edx")

/* Restore trap. This routine recovers the stack pointer into esp and retrieves
 * 'idtr', EFLAGS and general purpose registers from stack.
 *
 * This routine is called from the bootloader to restore the execution context
 * from before entering in sleep mode.
 */
#define qm_x86_restore_context(_restore_label, stack_pointer)                  \
	__asm__ __volatile__(#_restore_label ":\n\t"                           \
					     "lea %[stackpointer], %%eax\n\t"  \
					     "movl (%%eax), %%esp\n\t"         \
					     "popl %%edx\n\t"                  \
					     "movl %%edx, %%dr7\n\t"           \
					     "popl %%edx\n\t"                  \
					     "movl %%edx, %%dr6\n\t"           \
					     "popl %%edx\n\t"                  \
					     "movl %%edx, %%dr3\n\t"           \
					     "popl %%edx\n\t"                  \
					     "movl %%edx, %%dr2\n\t"           \
					     "popl %%edx\n\t"                  \
					     "movl %%edx, %%dr1\n\t"           \
					     "popl %%edx\n\t"                  \
					     "movl %%edx, %%dr0\n\t"           \
					     "popal\n\t"                       \
					     "popfl\n\t"                       \
					     "lidt (%%esp)\n\t"                \
					     "add $8, %%esp\n\t"               \
			     : /* Output operands. */                          \
			     : /* Input operands. */                           \
			     [stackpointer] "m"(stack_pointer)                 \
			     : /* Clobbered registers list. */                 \
			     "eax", "edx")

#else
#define qm_x86_set_resume_vector(_restore_label, shared_mem)
#define qm_x86_save_context(stack_pointer)
#define qm_x86_restore_context(_restore_label, stack_pointer)
#endif

/**
 * @}
 */

#endif /* __POWER_STATES_H__ */
