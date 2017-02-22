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

#ifndef __QM_SS_POWER_STATES_H__
#define __QM_SS_POWER_STATES_H__

#include "qm_common.h"
#include "qm_sensor_regs.h"

/**
 * SS Power mode control for Quark SE Microcontrollers.
 *
 * @defgroup groupSSPower SS Power states
 * @{
 */

/**
 * Sensor Subsystem SS1 Timers mode type.
 */
typedef enum {
	QM_SS_POWER_CPU_SS1_TIMER_OFF = 0, /**< Disable SS Timers in SS1. */
	QM_SS_POWER_CPU_SS1_TIMER_ON /**< Keep SS Timers enabled in SS1. */
} qm_ss_power_cpu_ss1_mode_t;

/**
 * Enable LPSS state entry.
 *
 * Put the SoC into LPSS on next C2/C2LP and SS2 state combination.<BR>
 * This function needs to be called on the Sensor Core to
 * Clock Gate ADC, I2C0, I2C1, SPI0 and SPI1 sensor peripherals.<BR>
 * Clock Gating sensor peripherals is a requirement to enter LPSS state.<BR>
 * After LPSS, qm_ss_power_soc_lpss_disable needs to be called to
 * restore clock gating.<BR>
 *
 * This needs to be called before any transition to C2/C2LP and SS2
 * in order to enter LPSS.<BR>
 * SoC Hybrid Clock is gated in this state.<BR>
 * Core Well Clocks are gated.<BR>
 * RTC is the only clock running.
 *
 * Possible SoC wake events are:
 * 	- Low Power Comparator Interrupt
 * 	- AON GPIO Interrupt
 * 	- AON Timer Interrupt
 * 	- RTC Interrupt
 */
void qm_ss_power_soc_lpss_enable(void);

#if (ENABLE_RESTORE_CONTEXT)
/**
 * Enter SoC sleep state and restore after wake up.
 *
 * Put the ARC core into sleep state until next SoC wake event
 * and continue execution after wake up where the application stopped.
 *
 * If the library is built with ENABLE_RESTORE_CONTEXT=1, then this function
 * will use the arc_restore_addr to save restore trap address which brings back
 * the ARC CPU to the point where this function was called.
 * This means that applications should refrain from using them.
 *
 * This function calls qm_ss_save_context and qm_ss_restore_context
 * in order to restore execution where it stopped.
 * All power management transitions are done by qm_power_soc_sleep().
 */
void qm_ss_power_soc_sleep_restore(void);
/**
 * Enter SoC sleep state and restore after wake up.
 *
 * Put the ARC core into sleep state until next SoC wake event
 * and continue execution after wake up where the application stopped.
 *
 * If the library is built with ENABLE_RESTORE_CONTEXT=1, then this function
 * will use the arc_restore_addr to save restore trap address which brings back
 * the ARC CPU to the point where this function was called.
 * This means that applications should refrain from using them.
 *
 * This function calls qm_ss_save_context and qm_ss_restore_context
 * in order to restore execution where it stopped.
 * All power management transitions are done by power_qm_soc_deep_sleep().
 */
void qm_ss_power_soc_deep_sleep_restore(void);

/**
 * Save context, enter ARC SS1 power save state and restore after wake up.
 *
 * This routine is same as qm_ss_power_soc_sleep_restore(), just instead of
 * going to sleep it will go to SS1 power save state.
 * Note: this function has a while(1) which will spin until we enter
 * (and exit) sleep and the power state change will be managed by the other
 * core.
 */
void qm_ss_power_sleep_wait(void);

/**
 * Enable the SENSOR startup restore flag.
 */
void qm_power_soc_set_ss_restore_flag(void);

#endif /* ENABLE_RESTORE_CONTEXT */

/**
 * Disable LPSS state entry.
 *
 * Clear LPSS enable flag.<BR>
 * Disable Clock Gating of ADC, I2C0, I2C1, SPI0 and SPI1 sensor
 * peripherals.<BR>
 * This will prevent entry in LPSS when cores are in C2/C2LP and SS2 states.
 */
void qm_ss_power_soc_lpss_disable(void);

/**
 * Enter Sensor SS1 state.
 *
 * Put the Sensor Subsystem into SS1.<BR>
 * Processor Clock is gated in this state.
 *
 * A wake event causes the Sensor Subsystem to transition to SS0.<BR>
 * A wake event is a sensor subsystem interrupt.
 *
 * According to the mode selected, Sensor Subsystem Timers can be disabled.
 *
 * @param[in] mode Mode selection for SS1 state.
 */
void qm_ss_power_cpu_ss1(const qm_ss_power_cpu_ss1_mode_t mode);

/**
 * Enter Sensor SS2 state or SoC LPSS state.
 *
 * Put the Sensor Subsystem into SS2.<BR>
 * Sensor Complex Clock is gated in this state.<BR>
 * Sensor Peripherals are gated in this state.<BR>
 *
 * This enables entry in LPSS if:
 *  - Sensor Subsystem is in SS2
 *  - Lakemont is in C2 or C2LP
 *  - LPSS entry is enabled
 *
 * A wake event causes the Sensor Subsystem to transition to SS0.<BR>
 * There are two kinds of wake event depending on the Sensor Subsystem
 * and SoC state:
 *  - SS2: a wake event is a Sensor Subsystem interrupt
 *  - LPSS: a wake event is a Sensor Subsystem interrupt or a Lakemont interrupt
 *
 * LPSS wake events apply if LPSS is entered.
 * If Host wakes the SoC from LPSS,
 * Sensor also transitions back to SS0.
 */
void qm_ss_power_cpu_ss2(void);

#if (ENABLE_RESTORE_CONTEXT) && (!UNIT_TEST)
/**
 * Save resume vector.
 *
 * Saves the resume vector in the global "arc_restore_addr" location.
 * The ARC will jump to the resume vector once a wake up event is
 * triggered and x86 resumes the ARC.
 */
#define qm_ss_set_resume_vector(_restore_label, arc_restore_addr)              \
	__asm__ __volatile__("mov r0, @arc_restore_addr\n\t"                   \
			     "st " #_restore_label ", [r0]\n\t"                \
			     : /* Output operands. */                          \
			     : /* Input operands. */                           \
			     : /* Clobbered registers list. */                 \
			     "r0")

/* Save execution context.
 *
 * This routine saves CPU registers onto cpu_context,
 * array.
 *
 */
#define qm_ss_save_context(cpu_context)                                        \
	__asm__ __volatile__("push_s r0\n\t"                                   \
			     "mov r0, @cpu_context\n\t"                        \
			     "st r1, [r0, 4]\n\t"                              \
			     "st r2, [r0, 8]\n\t"                              \
			     "st r3, [r0, 12]\n\t"                             \
			     "st r4, [r0, 16]\n\t"                             \
			     "st r5, [r0, 20]\n\t"                             \
			     "st r6, [r0, 24]\n\t"                             \
			     "st r7, [r0, 28]\n\t"                             \
			     "st r8, [r0, 32]\n\t"                             \
			     "st r9, [r0, 36]\n\t"                             \
			     "st r10, [r0, 40]\n\t"                            \
			     "st r11, [r0, 44]\n\t"                            \
			     "st r12, [r0, 48]\n\t"                            \
			     "st r13, [r0, 52]\n\t"                            \
			     "st r14, [r0, 56]\n\t"                            \
			     "st r15, [r0, 60]\n\t"                            \
			     "st r16, [r0, 64]\n\t"                            \
			     "st r17, [r0, 68]\n\t"                            \
			     "st r18, [r0, 72]\n\t"                            \
			     "st r19, [r0, 76]\n\t"                            \
			     "st r20, [r0, 80]\n\t"                            \
			     "st r21, [r0, 84]\n\t"                            \
			     "st r22, [r0, 88]\n\t"                            \
			     "st r23, [r0, 92]\n\t"                            \
			     "st r24, [r0, 96]\n\t"                            \
			     "st r25, [r0, 100]\n\t"                           \
			     "st r26, [r0, 104]\n\t"                           \
			     "st r27, [r0, 108]\n\t"                           \
			     "st r28, [r0, 112]\n\t"                           \
			     "st r29, [r0, 116]\n\t"                           \
			     "st r30, [r0, 120]\n\t"                           \
			     "st r31, [r0, 124]\n\t"                           \
			     "lr r31, [ic_ctrl]\n\t"                           \
			     "st r31, [r0, 128]\n\t"                           \
			     : /* Output operands. */                          \
			     : /* Input operands. */                           \
			     [ic_ctrl] "i"(QM_SS_AUX_IC_CTRL)                  \
			     : /* Clobbered registers list. */                 \
			     "r0")

/* Restore execution context.
 *
 * This routine restores CPU registers from cpu_context,
 * array.
 *
 * This routine is called from the bootloader to restore the execution context
 * from before entering in sleep mode.
 */
#define qm_ss_restore_context(_restore_label, cpu_context)                     \
	__asm__ __volatile__(                                                  \
	    #_restore_label                                                    \
	    ":\n\t"                                                            \
	    "mov r0, @cpu_context\n\t"                                         \
	    "ld r1, [r0, 4]\n\t"                                               \
	    "ld r2, [r0, 8]\n\t"                                               \
	    "ld r3, [r0, 12]\n\t"                                              \
	    "ld r4, [r0, 16]\n\t"                                              \
	    "ld r5, [r0, 20]\n\t"                                              \
	    "ld r6, [r0, 24]\n\t"                                              \
	    "ld r7, [r0, 28]\n\t"                                              \
	    "ld r8, [r0, 32]\n\t"                                              \
	    "ld r9, [r0, 36]\n\t"                                              \
	    "ld r10, [r0, 40]\n\t"                                             \
	    "ld r11, [r0, 44]\n\t"                                             \
	    "ld r12, [r0, 48]\n\t"                                             \
	    "ld r13, [r0, 52]\n\t"                                             \
	    "ld r14, [r0, 56]\n\t"                                             \
	    "ld r15, [r0, 60]\n\t"                                             \
	    "ld r16, [r0, 64]\n\t"                                             \
	    "ld r17, [r0, 68]\n\t"                                             \
	    "ld r18, [r0, 72]\n\t"                                             \
	    "ld r19, [r0, 76]\n\t"                                             \
	    "ld r20, [r0, 80]\n\t"                                             \
	    "ld r21, [r0, 84]\n\t"                                             \
	    "ld r22, [r0, 88]\n\t"                                             \
	    "ld r23, [r0, 92]\n\t"                                             \
	    "ld r24, [r0, 96]\n\t"                                             \
	    "ld r25, [r0, 100]\n\t"                                            \
	    "ld r26, [r0, 104]\n\t"                                            \
	    "ld r27, [r0, 108]\n\t"                                            \
	    "ld r28, [r0, 112]\n\t"                                            \
	    "ld r29, [r0, 116]\n\t"                                            \
	    "ld r30, [r0, 120]\n\t"                                            \
	    "ld r31, [r0, 124]\n\t"                                            \
	    "ld r0, [r0, 128]\n\t"                                             \
	    "sr r0, [ic_ctrl]\n\t"                                             \
	    "pop_s r0\n\t"                                                     \
	    "sr 0,[0x101]\n\t" /* Setup Sensor Subsystem TimeStamp Counter */  \
	    "sr 0,[0x100]\n\t"                                                 \
	    "sr -1,[0x102]\n\t"                                                \
	    : /* Output operands. */                                           \
	    : /* Input operands. */                                            \
	    [ic_ctrl] "i"(QM_SS_AUX_IC_CTRL)                                   \
	    : /* Clobbered registers list. */                                  \
	    "r0")
#else
#define qm_ss_set_resume_vector(_restore_label, arc_restore_addr)
#define qm_ss_save_context(cpu_context)
#define qm_ss_restore_context(_restore_label, cpu_context)
#endif

/**
 * @}
 */

#endif /* __QM_SS_POWER_STATES_H__ */
