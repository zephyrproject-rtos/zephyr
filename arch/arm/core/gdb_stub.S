/* gdb_stub.S - extra work performed upon exception entry/exit for GDB */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

Prep work done when entering exceptions consists of saving the callee-saved
registers before they get used by exception handlers, and recording the fact
that we are running in an exception.

Upon exception exit, it must be recorded that the task is not in an exception
anymore.
*/

#define _ASMLANGUAGE

#include <offsets.h>
#include <toolchain.h>
#include <sections.h>
#include <nano_private.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

/*******************************************************************************
*
* _GdbStubExcEntry - exception entry extra work when GDB_INFO is enabled
*
* During normal system operation, the callee-saved registers are saved lazily
* only when a context switch is required. To allow looking at the current
* threads registers while debugging an exception/interrupt, they must be saved
* upon entry since the handler could be using them: thus, looking at the CPU
* registers would show the current system state and not the current *thread*'s
* state.
*
* Also, record the fact that the thread is currently interrupted so that VQEMU
* looks into the CCS and not the CPU registers to obtain the current thread's
* register values.
*
* NOTE:
* - must be called with interrupts locked
* - cannot use r0 without saving it first
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _GdbStubExcEntry)

    ldr r1, =_nanokernel
    ldr r2, [r1, #__tNANO_flags_OFFSET]

    /* already in an exception, do not update the registers */
    ands r3, r2, #EXC_ACTIVE
    it ne
	bxne lr

    orrs r2, #EXC_ACTIVE
    str r2, [r1, #__tNANO_flags_OFFSET]
    ldr r1, [r1, #__tNANO_current_OFFSET]
    str r2, [r1, #__tCCS_flags_OFFSET]

    /* save callee-saved + psp in CCS */
    adds r1, #__tCCS_preempReg_OFFSET
    mrs ip, PSP
    stmia r1, {v1-v8, ip}

    bx lr

/*******************************************************************************
*
* _GdbStubExcExit - exception exit extra clean up when GDB_INFO is enabled
*
* Record the fact that the thread is not interrupted anymore so that VQEMU
* looks at the CPU registers and not into the CCS to obtain the current
* thread's register values. Only do this if this is not a nested exception.
*
* NOTE:
* - must be called with interrupts locked
* - cannot use r0 without saving it first
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _GdbStubExcExit)

    /* if we're nested (ie. !RETTOBASE), do not reset EXC_ACTIVE */
    ldr r1, =_SCS_ICSR
    ldr r1, [r1]
    ands r1, #_SCS_ICSR_RETTOBASE
    it eq
	bxeq lr

    ldr r1, =_nanokernel
    ldr r2, [r1, #__tNANO_flags_OFFSET]

    bic r2, #EXC_ACTIVE
    str r2, [r1, #__tNANO_flags_OFFSET]
    ldr r1, [r1, #__tNANO_current_OFFSET]
    str r2, [r1, #__tCCS_flags_OFFSET]

    bx lr

/*******************************************************************************
*
* _irq_vector_table_entry_with_gdb_stub - stub for ISRs installed directly in
*                               vector table
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
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _irq_vector_table_entry_with_gdb_stub)

    _GDB_STUB_EXC_ENTRY

    mrs r0, IPSR	/* get exception number */
    sub r0, r0, #16	/* get IRQ number */
    ldr r1, =_irq_vector_table

    /* grab real ISR at address: r1 + (r0 << 2) (table is 4-byte wide) */
    ldr r1, [r1, r0, LSL #2]

    /* jump to ISR, no return: ISR is responsible for calling _IntExit */
    bx r1
