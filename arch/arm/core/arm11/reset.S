/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 * Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM1176JZF-S reset handler
 *
 * Reset handler that prepares the ARM1176 CPU for running C code.
 * Normalizes CPU state inherited from firmware before Zephyr stack setup.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>
#include <cortex_a_r/tcm.h>
#include "vector_table.h"
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_reset)
GDATA(z_interrupt_stacks)
GDATA(z_arm_svc_stack)
GDATA(z_arm_sys_stack)
GDATA(z_arm_fiq_stack)
GDATA(z_arm_abort_stack)
GDATA(z_arm_undef_stack)
#if defined(CONFIG_SOC_RESET_HOOK)
GTEXT(soc_reset_hook)
#endif

/**
 * @brief Reset vector
 *
 * Entered in SVC mode with IRQ/FIQ disabled.  Firmware handoff may leave the
 * MMU, caches, and high-vector setting in an unknown state; normalize before
 * Zephyr stack and vector setup.
 */
SECTION_SUBSEC_FUNC(TEXT, _reset_section, z_arm_reset)
SECTION_SUBSEC_FUNC(TEXT, _reset_section, __start)

    /*
     * Normalize the CPU from firmware handoff.
     *
     * Force SVC32 mode with IRQ/FIQ masked, invalidate caches and TLBs,
     * then clear SCTLR bits that Zephyr must control explicitly (MMU, I/D
     * cache, high vectors, alignment, Thumb exception model).
     */
    mrs r0, cpsr
    bic r0, r0, #0x3f
    orr r0, r0, #(MODE_SVC | I_BIT | F_BIT)
    msr cpsr_c, r0

    mov r0, #0
    mcr p15, 0, r0, c7, c7, 0      /* invalidate I/D caches */
    mcr p15, 0, r0, c8, c7, 0      /* invalidate unified TLB */
    data_sync_barrier
    instr_sync_barrier

    mrc p15, 0, r0, c1, c0, 0
    bic r0, r0, #0x40000000        /* clear TE (Thumb exceptions) */
    bic r0, r0, #0x03000000        /* clear VE/EE */
    bic r0, r0, #0x00200000        /* clear FI (low interrupt latency) */
    bic r0, r0, #0x00000030        /* clear legacy P/D state */
    bic r0, r0, #0x2000            /* clear V (high vectors) */
    bic r0, r0, #0x0200            /* clear RR (round-robin replacement) */
    bic r0, r0, #0x0100            /* clear L4 */
    bic r0, r0, #0x0080            /* clear I (instruction cache) */
    bic r0, r0, #0x0004            /* clear C (data/unified cache) */
    bic r0, r0, #0x0001            /* clear M (MMU) */
    orr r0, r0, #0x00000030        /* force 32-bit exception/address model */
    orr r0, r0, #0x00000002        /* keep alignment fault checking enabled */
    mcr p15, 0, r0, c1, c0, 0
    data_sync_barrier
    instr_sync_barrier

    ldr r4, =z_prep_c
    ldr r5, =(z_arm_fiq_stack + CONFIG_ARMV7_FIQ_STACK_SIZE)
    ldr r6, =(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
    ldr r7, =(z_arm_abort_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE)
    ldr r8, =(z_arm_undef_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE)
    ldr r9, =(z_arm_svc_stack + CONFIG_ARMV7_SVC_STACK_SIZE)
    ldr r10, =(z_arm_sys_stack + CONFIG_ARMV7_SYS_STACK_SIZE)

    /* Configure mode stacks */

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

#if defined(CONFIG_SOC_RESET_HOOK)
    bl soc_reset_hook
#endif

#if defined(CONFIG_WDOG_INIT)
    bl z_arm_watchdog_init
#endif

#if defined(CONFIG_DISABLE_TCM_ECC)
    bl z_arm_tcm_disable_ecc
#endif

    bl z_arm_relocate_vector_table

    bx r4
