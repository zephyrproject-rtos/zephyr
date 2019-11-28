/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_reset)
GTEXT(memset)
GDATA(_interrupt_stack)
#if defined(CONFIG_PLATFORM_SPECIFIC_INIT)
GTEXT(z_platform_init)
#endif

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
 * MSP is to be set up to point to the one-and-only interrupt stack during
 * later boot. That would not be possible if in use for running C code.
 *
 * When these steps are completed, jump to z_arm_prep_c(), which will finish
 * setting up the system for running C code.
 *
 * @return N/A
 */

SECTION_SUBSEC_FUNC(TEXT,_reset_section,z_arm_reset)

/*
 * The entry point is located at the z_arm_reset symbol, which
 * is fetched by a XIP image playing the role of a bootloader, which jumps to
 * it, not through the reset vector mechanism. Such bootloaders might want to
 * search for a __start symbol instead, so create that alias here.
 */
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__start)

#if defined(CONFIG_PLATFORM_SPECIFIC_INIT)
    bl z_platform_init
#endif

    /* lock interrupts: will get unlocked when switch to main task */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    cpsid i
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI, r0
#else
#error Unknown ARM architecture
#endif

#ifdef CONFIG_WDOG_INIT
    /* board-specific watchdog initialization is necessary */
    bl z_arm_watchdog_init
#endif

#ifdef CONFIG_INIT_STACKS
    ldr r0, =_interrupt_stack
    ldr r1, =0xaa
    ldr r2, =CONFIG_ISR_STACK_SIZE
    bl memset
#endif

    /*
     * Set PSP and use it to boot without using MSP, so that it
     * gets set to _interrupt_stack during initialization.
     */
    ldr r0, =_interrupt_stack
    ldr r1, =CONFIG_ISR_STACK_SIZE
    adds r0, r0, r1
    msr PSP, r0
    mrs r0, CONTROL
    movs r1, #2
    orrs r0, r1 /* CONTROL_SPSEL_Msk */
    msr CONTROL, r0
    /*
     * When changing the stack pointer, software must use an ISB instruction
     * immediately after the MSR instruction. This ensures that instructions
     * after the ISB instruction execute using the new stack pointer.
     */
    isb

    /*
     * 'bl' jumps the furthest of the branch instructions that are
     * supported on all platforms. So it is used when jumping to z_arm_prep_c
     * (even though we do not intend to return).
     */
    bl z_arm_prep_c
