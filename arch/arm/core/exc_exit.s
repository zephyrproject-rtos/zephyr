/* exc_exit.s - ARM CORTEX-M3 exception/interrupt exit API */

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

Provides functions for performing kernel handling when exiting exceptions or
interrupts that are installed directly in the vector table (i.e. that are not
wrapped around by _isr_wrapper()).
*/

#define _ASMLANGUAGE

#include <nanok.h>
#include <offsets.h>
#include <toolchain.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(_ExcExit)
GTEXT(_IntExit)
GDATA(_nanokernel)

#if CONFIG_GDB_INFO
  #define _EXIT_EXC_IF_FIBER_PREEMPTED beq _ExcExitWithGdbStub
#else
  _EXIT_EXC_IF_FIBER_PREEMPTED:      .macro
                                     it eq
                                     bxeq lr
                                     .endm
#endif
#define _EXIT_EXC_IF_FIBER_NOT_READY _EXIT_EXC_IF_FIBER_PREEMPTED

/*******************************************************************************
*
* _IntExit - kernel housekeeping when exiting interrupt handler installed
*            directly in vector table
*
* Kernel allows installing interrupt handlers (ISRs) directly into the vector
* table to get the lowest interrupt latency possible. This allows the ISR to be
* invoked directly without going through a software interrupt table. However,
* upon exiting the ISR, some kernel work must still be performed, namely
* possible context switching. While ISRs connected in the software interrupt
* table do this automatically via a wrapper, ISRs connected directly in the
* vector table must invoke _IntExit() as the *very last* action before
* returning.
*
* e.g.
*
* void myISR(void)
*     {
*     printk("in %s\n", __FUNCTION__);
*     doStuff();
*     _IntExit();
*     }
*
* RETURNS: N/A
*/

SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, _IntExit)

/* _IntExit falls through to _ExcExit (they are aliases of each other) */


/*******************************************************************************
*
* _ExcExit - kernel housekeeping when exiting exception handler installed
*            directly in vector table
*
* See _IntExit().
*
* RETURNS: N/A
*/

SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, _ExcExit)

    ldr r1, =_nanokernel

    /* is the current thread preemptible (task) ? */
    ldr r2, [r1, #__tNANO_flags_OFFSET]
    ands.w r2, #PREEMPTIBLE
    _EXIT_EXC_IF_FIBER_PREEMPTED

    /* is there a fiber ready ? */
    ldr r2, [r1, #__tNANO_fiber_OFFSET]
    cmp r2, #0
    _EXIT_EXC_IF_FIBER_NOT_READY

    /* context switch required, pend the PendSV exception */
    ldr r1, =_SCS_ICSR
    ldr r2, =_SCS_ICSR_PENDSV
    str r2, [r1]

_ExcExitWithGdbStub:

    _GDB_STUB_EXC_EXIT

    bx lr
