/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 * Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>
#include <cortex_a_r/tcm.h>
#include "vector_table.h"
#include "boot.h"
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
#if defined(CONFIG_AARCH32_ARMV8_R)
    /* Check if we are starting in HYP mode */
    mrs r0, cpsr
    and r0, r0, #MODE_MASK
    cmp r0, #MODE_HYP
    bne EL1_Reset_Handler

    /*
     * The HSCTLR register provides top-level control of system operation in Hyp mode.
     * Since the OS is not running in Hyp mode, and considering the Armv8-R AArch32
     * architecture profile, there's no need to modify HSCTLR configuration unless
     * Fast Interrupts need to be enabled.
     */

    /* Init HACTLR: Enable EL1 access to all IMP DEF registers */
    ldr r0, =HACTLR_INIT
    mcr p15, 4, r0, c1, c0, 1

   /* Go to SVC mode */
    mrs r0, cpsr
    bic r0, #MODE_MASK
    orr r0, #MODE_SVC
    msr spsr_cxsf, r0

    ldr r0, =EL1_Reset_Handler
    msr elr_hyp, r0
    dsb
    isb
    eret

EL1_Reset_Handler:

#endif

#if defined(CONFIG_DCLS)
    /*
     * Initialise CPU registers to a defined state if the processor is
     * configured as Dual-redundant Core Lock-step (DCLS). This is required
     * for state convergence of the two parallel executing cores.
     */

    /* Common and SVC mode registers */
    mov r0,  #0
    mov r1,  #0
    mov r2,  #0
    mov r3,  #0
    mov r4,  #0
    mov r5,  #0
    mov r6,  #0
    mov r7,  #0
    mov r8,  #0
    mov r9,  #0
    mov r10, #0
    mov r11, #0
    mov r12, #0
    mov r13, #0         /* r13_svc */
    mov r14, #0         /* r14_svc */
    mrs r0,  cpsr
    msr spsr_cxsf, r0   /* spsr_svc */

    /* FIQ mode registers */
    cps #MODE_FIQ
    mov r8,  #0         /* r8_fiq */
    mov r9,  #0         /* r9_fiq */
    mov r10, #0         /* r10_fiq */
    mov r11, #0         /* r11_fiq */
    mov r12, #0         /* r12_fiq */
    mov r13, #0         /* r13_fiq */
    mov r14, #0         /* r14_fiq */
    mrs r0,  cpsr
    msr spsr_cxsf, r0   /* spsr_fiq */

    /* IRQ mode registers */
    cps #MODE_IRQ
    mov r13, #0         /* r13_irq */
    mov r14, #0         /* r14_irq */
    mrs r0,  cpsr
    msr spsr_cxsf, r0   /* spsr_irq */

    /* ABT mode registers */
    cps #MODE_ABT
    mov r13, #0         /* r13_abt */
    mov r14, #0         /* r14_abt */
    mrs r0,  cpsr
    msr spsr_cxsf, r0   /* spsr_abt */

    /* UND mode registers */
    cps #MODE_UND
    mov r13, #0         /* r13_und */
    mov r14, #0         /* r14_und */
    mrs r0,  cpsr
    msr spsr_cxsf, r0   /* spsr_und */

    /* SYS mode registers */
    cps #MODE_SYS
    mov r13, #0         /* r13_sys */
    mov r14, #0         /* r14_sys */

#if defined(CONFIG_FPU) && defined(CONFIG_CPU_HAS_VFP)
    /*
     * Initialise FPU registers to a defined state.
     */

    /* Allow VFP coprocessor access */
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #(CPACR_CP10(CPACR_FA) | CPACR_CP11(CPACR_FA))
    mcr p15, 0, r0, c1, c0, 2

    /* Enable VFP */
    mov  r0, #FPEXC_EN
    vmsr fpexc, r0

    /* Initialise VFP registers */
    fmdrr d0,  r1, r1
    fmdrr d1,  r1, r1
    fmdrr d2,  r1, r1
    fmdrr d3,  r1, r1
    fmdrr d4,  r1, r1
    fmdrr d5,  r1, r1
    fmdrr d6,  r1, r1
    fmdrr d7,  r1, r1
    fmdrr d8,  r1, r1
    fmdrr d9,  r1, r1
    fmdrr d10, r1, r1
    fmdrr d11, r1, r1
    fmdrr d12, r1, r1
    fmdrr d13, r1, r1
    fmdrr d14, r1, r1
    fmdrr d15, r1, r1
#if defined(CONFIG_VFP_FEATURE_REGS_S64_D32)
    fmdrr d16, r1, r1
    fmdrr d17, r1, r1
    fmdrr d18, r1, r1
    fmdrr d19, r1, r1
    fmdrr d20, r1, r1
    fmdrr d21, r1, r1
    fmdrr d22, r1, r1
    fmdrr d23, r1, r1
    fmdrr d24, r1, r1
    fmdrr d25, r1, r1
    fmdrr d26, r1, r1
    fmdrr d27, r1, r1
    fmdrr d28, r1, r1
    fmdrr d29, r1, r1
    fmdrr d30, r1, r1
    fmdrr d31, r1, r1
#endif /* CONFIG_VFP_FEATURE_REGS_S64_D32 */

    vmsr fpscr, r1
    vmsr fpexc, r1
#endif /* CONFIG_FPU && CONFIG_CPU_HAS_VFP */

#endif /* CONFIG_DCLS */

#if defined(CONFIG_CPU_CORTEX_R52_CACHE_SEGREGATION)
    ldr r0, =IMP_CSCTLR(CONFIG_CPU_CORTEX_R52_ICACHE_FLASH_WAY,
                        CONFIG_CPU_CORTEX_R52_DCACHE_FLASH_WAY)
    mcr p15, 1, r0, c9, c1, 0
#endif

    ldr r0, =arm_cpu_boot_params

#if CONFIG_MP_MAX_NUM_CPUS > 1
    /*
     * This code uses voting locks, like arch/arm64/core/reset.S, to determine primary CPU.
     */

    /*
     * Get the "logic" id defined by cpu_node_list statically for voting lock self-identify.
     * It is worth noting that this is NOT the final logic id (arch_curr_cpu()->id)
     */
    get_cpu_logic_id r1, r2, r3, r4 // r1: MPID, r2: logic id

    add r4, r0, #BOOT_PARAM_VOTING_OFFSET

    /* signal our desire to vote */
    mov r5, #1
    strb r5, [r4, r2]
    ldr r3, [r0, #BOOT_PARAM_MPID_OFFSET]
    cmn r3, #1
    beq 1f

    /* some core already won, release */
    mov r7, #0
    strb r7, [r4, r2]
    b _secondary_core

    /* suggest current core then release */
1:  str r1, [r0, #BOOT_PARAM_MPID_OFFSET]
    strb r7, [r4, r2]
    dmb

    /* then wait until every core else is done voting */
    mov r5, #0
2:  ldrb r3, [r4, r5]
    tst r3, #255
    /* wait */
    bne 2b
    add r5, r5, #1
    cmp r5, #CONFIG_MP_MAX_NUM_CPUS
    bne 2b

    /* check if current core won */
    dmb
    ldr r3, [r0, #BOOT_PARAM_MPID_OFFSET]
    cmp r3, r1
    beq _primary_core
    /* fallthrough secondary */

    /* loop until our turn comes */
_secondary_core:
    dmb
    ldr r2, [r0, #BOOT_PARAM_MPID_OFFSET]
    cmp r1, r2
    bne _secondary_core

    /* we can now load our stack pointer values and move on */
    ldr r4, =arch_secondary_cpu_init
    ldr r5, [r0, #BOOT_PARAM_FIQ_SP_OFFSET]
    ldr r6, [r0, #BOOT_PARAM_IRQ_SP_OFFSET]
    ldr r7, [r0, #BOOT_PARAM_ABT_SP_OFFSET]
    ldr r8, [r0, #BOOT_PARAM_UDF_SP_OFFSET]
    ldr r9, [r0, #BOOT_PARAM_SVC_SP_OFFSET]
    ldr r10, [r0, #BOOT_PARAM_SYS_SP_OFFSET]
    b 2f

_primary_core:
#endif

    ldr r4, =z_prep_c
    ldr r5, =(z_arm_fiq_stack + CONFIG_ARMV7_FIQ_STACK_SIZE)
    ldr r6, =(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
    ldr r7, =(z_arm_abort_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE)
    ldr r8, =(z_arm_undef_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE)
    ldr r9, =(z_arm_svc_stack + CONFIG_ARMV7_SVC_STACK_SIZE)
    ldr r10, =(z_arm_sys_stack + CONFIG_ARMV7_SYS_STACK_SIZE)

2:
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

#if defined(CONFIG_SOC_RESET_HOOK)
    /* Execute platform-specific initialisation if applicable */
    bl soc_reset_hook
#endif

#if defined(CONFIG_WDOG_INIT)
    /* board-specific watchdog initialization is necessary */
    bl z_arm_watchdog_init
#endif

#if defined(CONFIG_DISABLE_TCM_ECC)
    bl z_arm_tcm_disable_ecc
#endif

    bl z_arm_relocate_vector_table

    bx r4
