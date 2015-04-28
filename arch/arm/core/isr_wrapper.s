/* isr_wrapper.s - ARM CORTEX-M3 wrapper for ISRs with parameter  */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

Wrapper installed in vector table for handling dynamic interrupts that accept
a parameter.
*/

#define _ASMLANGUAGE

#include <offsets.h>
#include <toolchain.h>
#include <sections.h>
#include <sw_isr_table.h>
#include <nanok.h>
#include <nanokernel/cpu.h>

_ASM_FILE_PROLOGUE

GDATA(_IsrTable)

GTEXT(_isr_wrapper)
GTEXT(_IntExit)

/*******************************************************************************
*
* _isr_wrapper - wrapper around ISRs when inserted in software ISR table
*
* When inserted in the vector table, _isr_wrapper() demuxes the ISR table using
* the running interrupt number as the index, and invokes the registered ISR
* with its correspoding argument. When returning from the ISR, it determines
* if a context switch needs to happen (see documentation for __pendsv()) and
* pends the PendSV exception if so: the latter will perform the context switch
* itself.
*
* RETURNS: N/A
*/
SECTION_FUNC(TEXT, _isr_wrapper)

    _GDB_STUB_EXC_ENTRY

    push {lr}		/* lr is now the first item on the stack */

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
    /*
     * All interrupts are disabled when handling idle wakeup.
     * For tickless idle, this ensures that the calculation and programming of
     * the device for the next timer deadline is not interrupted.
     * For non-tickless idle, this ensures that the clearing of the kernel idle
     * state is not interrupted.
     * In each case, _sys_power_save_idle_exit is called with interrupts 
     * disabled.
     */
    cpsid i  /* PRIMASK = 1 */

    /* is this a wakeup from idle ? */
    ldr r2, =_NanoKernel
    ldr r0, [r2, #__tNANO_idle_OFFSET]  /* requested idle duration, in ticks */
    cmp r0, #0
    ittt ne
	movne	r1, #0
        strne	r1, [r2, #__tNANO_idle_OFFSET]  /* clear kernel idle state */
	blxne	_sys_power_save_idle_exit

    cpsie i		/* re-enable interrupts (PRIMASK = 0) */
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */

    mrs r0, IPSR	/* get exception number */
    sub r0, r0, #16	/* get IRQ number */
    lsl r0, r0, #3	/* table is 8-byte wide */
    ldr r1, =_IsrTable
    add r1, r1, r0	/* table entry: ISRs must have their MSB set to stay
			 *              in thumb mode */

    ldmia r1,{r0,r3}	/* arg in r0, ISR in r3 */
    blx r3		/* call ISR */

    pop {lr}

    /* exception return is done in _IntExit(), including _GDB_STUB_EXC_EXIT */
    b _IntExit
