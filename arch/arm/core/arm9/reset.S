/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#include <arm9/stack.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(__start)
GTEXT(z_arm_reset)

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is in Supervisor mode
 * and interrupts are disabled. The processor architectural registers are in
 * an indeterminate state.
 *
 * When these steps are completed, jump to z_prep_c(), which will finish
 * setting up the system for running C code.
 *
 */
SECTION_SUBSEC_FUNC(TEXT, _reset_section, z_arm_reset)
SECTION_SUBSEC_FUNC(TEXT, _reset_section, __start)

    ldr r5, =(z_arm_fiq_stack + CONFIG_ARMV5_FIQ_STACK_SIZE)
    ldr r6, =(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
    ldr r7, =(z_arm_abort_stack + CONFIG_ARMV5_EXCEPTION_STACK_SIZE)
    ldr r8, =(z_arm_undef_stack + CONFIG_ARMV5_EXCEPTION_STACK_SIZE)
    ldr r9, =(z_arm_svc_stack + CONFIG_ARMV5_SVC_STACK_SIZE)
    ldr r10, =(z_arm_sys_stack + CONFIG_ARMV5_SYS_STACK_SIZE)

    /*
     * Configure stack.
     */

    /* FIQ mode stack */
    msr CPSR_c, #(MODE_FIQ | I_BIT | F_BIT)
    mov sp, r5

    /* IRQ mode stack */
    msr CPSR_c, #(MODE_IRQ | I_BIT | F_BIT)
    mov sp, r6

    /* ABT mode stack */
    msr CPSR_c, #(MODE_ABT | I_BIT | F_BIT)
    mov sp, r7

    /* UND mode stack */
    msr CPSR_c, #(MODE_UND | I_BIT | F_BIT)
    mov sp, r8

    /* SVC mode stack */
    msr CPSR_c, #(MODE_SVC | I_BIT | F_BIT)
    mov sp, r9

    /* SYS mode stack */
    msr CPSR_c, #(MODE_SYS | I_BIT | F_BIT)
    mov sp, r10

    ldr r4, =z_prep_c
    bx r4
