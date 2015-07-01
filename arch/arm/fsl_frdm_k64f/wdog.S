/* wdog.S - watchdog initialization for fsl_frdm_k64f BSP */

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
This module initializes the watchdog for the fsl_frdm_k64f BSP.
 */

#define _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <sections.h>

_ASM_FILE_PROLOGUE

GTEXT(_WdogInit)

/* watchdog register offsets */
#define WDOG_SCTRL_HI_OFFSET  0x0
#define WDOG_UNLOCK_OFFSET  0xE

/* watchdog command words */
#define WDOG_UNLOCK_1_CMD  0xC520
#define WDOG_UNLOCK_2_CMD  0xD928

/**
 *
 * _WdogInit - Watchdog timer disable routine
 *
 * This routine will disable the watchdog timer.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT,_WdogInit)
    /*
     * NOTE: DO NOT SINGLE STEP THROUGH THIS FUNCTION!!!
     * There are timing requirements for the execution of the unlock process.
     * Single stepping through the code will cause the CPU to reset.
     */

    /*
     * First unlock the watchdog so that we can write to registers.
     *
     * This sequence must execute within 20 clock cycles, so disable
     * interrupts to keep the code atomic and ensure the timing.
     */

    cpsid i

    ldr r0, =PERIPH_ADDR_BASE_WDOG

    movw r1, #WDOG_UNLOCK_1_CMD
    strh r1, [r0, #WDOG_UNLOCK_OFFSET]

    movw r1, #WDOG_UNLOCK_2_CMD
    strh r1, [r0, #WDOG_UNLOCK_OFFSET]

    /*
     * Disable the watchdog.
     *
     * Writes to control/configuration registers must execute within
     * 256 clock cycles after unlocking.
     */

    ldrh r1, [r0, #WDOG_SCTRL_HI_OFFSET]
    mov  r2, #1
    bics r1, r2
    strh r1, [r0, #WDOG_SCTRL_HI_OFFSET]

    cpsie i

    bx lr

