/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Extra work performed upon exception entry/exit for GDB
 *
 *
 * Prep work done when entering exceptions consists of saving the callee-saved
 * registers before they get used by exception handlers, and recording the fact
 * that we are running in an exception.
 *
 * Upon exception exit, it must be recorded that the task is not in an exception
 * anymore.
 */

#define _ASMLANGUAGE

#include <offsets_short.h>
#include <toolchain.h>
#include <sections.h>
#include <kernel_structs.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

/**
 *
 * @brief Exception entry extra work when GDB_INFO is enabled
 *
 * During normal system operation, the callee-saved registers are saved lazily
 * only when a context switch is required. To allow looking at the current
 * threads registers while debugging an exception/interrupt, they must be saved
 * upon entry since the handler could be using them: thus, looking at the CPU
 * registers would show the current system state and not the current *thread*'s
 * state.
 *
 * Also, record the fact that the thread is currently interrupted so that VQEMU
 * looks into the TCS and not the CPU registers to obtain the current thread's
 * register values.
 *
 * NOTE:
 * - must be called with interrupts locked
 * - cannot use r0 without saving it first
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _GdbStubExcEntry)

    ldr r1, =_kernel
    ldr r2, [r1, #__tNANO_flags_OFFSET]

    /* already in an exception, do not update the registers */
    ldr r3, =EXC_ACTIVE
    ands r3, r2
    beq _GdbStubEditReg
    bx lr

_GdbStubEditReg:
    ldr r3, =EXC_ACTIVE
    orrs r2, r3
    str r2, [r1, #__tNANO_flags_OFFSET]
    ldr r1, [r1, #__tNANO_current_OFFSET]
    str r2, [r1, #__tTCS_flags_OFFSET]

    /* save callee-saved + psp in TCS */
    adds r1, #__tTCS_preempReg_OFFSET
    mrs ip, PSP
#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
    /* Store current r4-r7 */
    stmea r1!, {r4-r7}
    /* copy r8-r12 into r3-r7 */
    mov r3, r8
    mov r4, r9
    mov r5, r10
    mov r6, r11
    mov r7, ip
    /* store r8-12 */
    stmea r1!, {r3-r7}
#else /* CONFIG_CPU_CORTEX_M0_M0PLUS */
    stmia r1, {v1-v8, ip}
#endif

    bx lr

/**
 *
 * @brief Exception exit extra clean up when GDB_INFO is enabled
 *
 * Record the fact that the thread is not interrupted anymore so that VQEMU
 * looks at the CPU registers and not into the TCS to obtain the current
 * thread's register values. Only do this if this is not a nested exception.
 *
 * NOTE:
 * - must be called with interrupts locked
 * - cannot use r0 without saving it first
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _GdbStubExcExit)

#if !defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
    /* if we're nested (ie. !RETTOBASE), do not reset EXC_ACTIVE */
    ldr r1, =_SCS_ICSR
    ldr r1, [r1]
    ands r1, #_SCS_ICSR_RETTOBASE
    it eq
	bxeq lr
#endif

    ldr r1, =_kernel
    ldr r2, [r1, #__tNANO_flags_OFFSET]

    ldr r3, =EXC_ACTIVE
    bics r2, r3
    str r2, [r1, #__tNANO_flags_OFFSET]
    ldr r1, [r1, #__tNANO_current_OFFSET]
    str r2, [r1, #__tTCS_flags_OFFSET]

    bx lr

/**
 *
 * @brief Stub for ISRs installed directly in vector table
 *
 * The kernel on Cortex-M3/4 can be configured so that ISRs
 * are installed directly in the vector table for maximum efficiency.
 *
 * When OS-awareness is enabled, a stub must be inserted to invoke
 * _GdbStubExcEntry() before the user ISR runs, to save the current task's
 * registers. This stub thus gets inserted in the vector table instead of the
 * user's ISR. The user's IRQ vector table gets pushed after the vector table
 * automatically by the linker script: this is all transparent to the user.
 * This stub must also act as a demuxer that find the running exception and
 * invoke the user's real ISR.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, _irq_vector_table_entry_with_gdb_stub)

    _GDB_STUB_EXC_ENTRY

    mrs r0, IPSR	/* get exception number */
    subs r0, #16	/* get IRQ number */
    ldr r1, =_irq_vector_table

    /* grab real ISR at address: r1 + (r0 << 2) (table is 4-byte wide) */
    lsls r3, r0, #2
    ldr r1, [r1, r3]

    /* jump to ISR, no return: ISR is responsible for calling _IntExit */
    bx r1
