/* fault_s.s - fault handlers for ARM Cortex-M */

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
Fault handlers for ARM Cortex-M processors.
*/

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>
#include <nanokernel/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(_Fault)

GTEXT(__hard_fault)
GTEXT(__mpu_fault)
GTEXT(__bus_fault)
GTEXT(__usage_fault)
GTEXT(__debug_monitor)
GTEXT(__reserved)

/*******************************************************************************
*
* __fault - fault handler installed in the fault and reserved vectors
*
* Entry point for the hard fault, MPU fault, bus fault, usage fault, debug
* monitor and reserved exceptions.
*
* Save the values of the MSP and PSP in r0 and r1 respectively, so the first
* and second parameters to the _Fault() C function that will handle the rest.
* This has to be done because at this point we do not know if the fault
* happened while handling an exception or not, and thus the ESF could be on
* either stack. _Fault() will find out where the ESF resides.
*
* Provides these symbols:
*
*   __hard_fault
*   __mpu_fault
*   __bus_fault
*   __usage_fault
*   __debug_monitor
*   __reserved
*/

SECTION_SUBSEC_FUNC(TEXT,__fault,__hard_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__mpu_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__bus_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__usage_fault)
SECTION_SUBSEC_FUNC(TEXT,__fault,__debug_monitor)
SECTION_SUBSEC_FUNC(TEXT,__fault,__reserved)

    _GDB_STUB_EXC_ENTRY

    /* force unlock interrupts */
    eors.n r0, r0
    msr BASEPRI, r0

    mrs r0, MSP
    mrs r1, PSP

    push {lr}
    bl _Fault

    _GDB_STUB_EXC_EXIT

    pop {pc}

    .end
