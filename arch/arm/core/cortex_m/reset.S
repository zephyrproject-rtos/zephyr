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
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#define _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

GTEXT(__reset)

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is in thread mode with
 * privileged level. At this point, the main stack pointer (MSP) is already
 * pointing to a valid area in SRAM.
 *
 * Locking interrupts prevents anything but NMIs and hard faults from
 * interrupting the CPU. A default NMI handler is already in place in the
 * vector table, and the boot code should not generate hard fault, or we're in
 * deep trouble.
 *
 * We want to use the process stack pointer (PSP) instead of the MSP, since the
 * MSP is to be set up to point to the one-and-only interrupt stack during later
 * boot. That would not be possible if in use for running C code.
 *
 * When these steps are completed, jump to _PrepC(), which will finish setting
 * up the system for running C code.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT,__reset)

    /* lock interrupts: will get unlocked when switch to main task */
    movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI, r0

    /*
     * Set PSP and use it to boot without using MSP, so that it
     * gets set to _interrupt_stack during nanoInit().
     */
    ldr r0, =__CORTEXM_BOOT_PSP
    msr PSP, r0
    movs.n r0, #2	/* switch to using PSP (bit1 of CONTROL reg) */
    msr CONTROL, r0

#ifdef CONFIG_WDOG_INIT
    /* board-specific watchdog initialization is necessary */
    bl _WdogInit
#endif

    b _PrepC
