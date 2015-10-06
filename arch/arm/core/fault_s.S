/* fault_s.S - fault handlers for ARM Cortex-M */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
Fault handlers for ARM Cortex-M processors.
 */

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(_Fault)

GTEXT(__hard_fault)
GTEXT(__mpu_fault)
GTEXT(__bus_fault)
GTEXT(__usage_fault)
GTEXT(__debug_monitor)
GTEXT(__reserved)

/**
 *
 * @brief Fault handler installed in the fault and reserved vectors
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
