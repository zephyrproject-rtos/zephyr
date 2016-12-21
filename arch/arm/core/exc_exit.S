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

/**
 * @file
 * @brief ARM CORTEX-M exception/interrupt exit API
 *
 *
 * Provides functions for performing kernel handling when exiting exceptions or
 * interrupts that are installed directly in the vector table (i.e. that are not
 * wrapped around by _isr_wrapper()).
 */

#define _ASMLANGUAGE

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(_ExcExit)
GTEXT(_IntExit)
GDATA(_kernel)

/**
 *
 * @brief Kernel housekeeping when exiting interrupt handler installed
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
 * @return N/A
 */

SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, _IntExit)

/* _IntExit falls through to _ExcExit (they are aliases of each other) */


/**
 *
 * @brief Kernel housekeeping when exiting exception handler installed
 *            directly in vector table
 *
 * See _IntExit().
 *
 * @return N/A
 */

SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, _ExcExit)

#ifdef CONFIG_PREEMPT_ENABLED
    ldr r0, =_kernel

    ldr r1, [r0, #_kernel_offset_to_current]
#ifdef CONFIG_CPU_CORTEX_M0_M0PLUS
    movs r3, #_thread_offset_to_prio
    ldrsb r2, [r1, r3]
#else
    ldrsb r2, [r1, #_thread_offset_to_prio]
#endif
    ldrb r3, [r1, #_thread_offset_to_sched_locked]

    /* coop thread ? do not schedule */
    cmp r2, #0
    blt _EXIT_EXC

    /* scheduler locked ? do not schedule */
    cmp r3, #0
    bne _EXIT_EXC

    ldr r0, [r0, _kernel_offset_to_ready_q_cache]
    cmp r0, r1
    beq _EXIT_EXC

    /* context switch required, pend the PendSV exception */
    ldr r1, =_SCS_ICSR
    ldr r2, =_SCS_ICSR_PENDSV
    str r2, [r1]

_ExcExitWithGdbStub:

_EXIT_EXC:
#endif /* CONFIG_PREEMPT_ENABLED */

    bx lr
