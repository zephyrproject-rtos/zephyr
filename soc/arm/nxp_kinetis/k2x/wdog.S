/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Watchdog initialization for fsl_frdm_k64f platform
 *
 * This module initializes the watchdog for the fsl_frdm_k64f platform.
 */

#include <soc.h>
#include <toolchain.h>
#include <linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_watchdog_init)

/* watchdog register offsets */
#define WDOG_SCTRL_HI_OFFSET  0x0
#define WDOG_UNLOCK_OFFSET  0xE

/* watchdog command words */
#define WDOG_UNLOCK_1_CMD  0xC520
#define WDOG_UNLOCK_2_CMD  0xD928

/**
 *
 * @brief Watchdog timer disable routine
 *
 * This routine will disable the watchdog timer.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT,z_arm_watchdog_init)
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

