/* reset_s.S - reset handler */

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
Reset handler that prepares the system for running C code.
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
