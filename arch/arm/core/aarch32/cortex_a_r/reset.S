/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
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
#include <offsets_short.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_reset)
GDATA(z_interrupt_stacks)
GDATA(z_arm_svc_stack)
GDATA(z_arm_sys_stack)
GDATA(z_arm_fiq_stack)
GDATA(z_arm_abort_stack)
GDATA(z_arm_undef_stack)
#if defined(CONFIG_PLATFORM_SPECIFIC_INIT)
GTEXT(z_platform_init)
#endif

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is in Supervisor mode
 * and interrupts are disabled. The processor architectural registers are in
 * an indeterminate state.
 *
 * When these steps are completed, jump to z_arm_prep_c(), which will finish
 * setting up the system for running C code.
 *
 * @return N/A
 */
SECTION_SUBSEC_FUNC(TEXT, _reset_section, z_arm_reset)
SECTION_SUBSEC_FUNC(TEXT, _reset_section, __start)

#if defined(CONFIG_CPU_HAS_DCLS)
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

#if defined(CONFIG_FPU)
    /*
     * Initialise FPU registers to a defined state.
     */

    /* Allow VFP coprocessor access */
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #(CPACR_CP10(CPACR_FA) | CPACR_CP11(CPACR_FA))
    mcr p15, 0, r0, c1, c0, 2

    /* Enable VFP */
    mov  r0, #FPEXC_EN
    fmxr fpexc, r0

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
#endif /* CONFIG_FPU */

#endif /* CONFIG_CPU_HAS_DCLS */

    /*
     * Configure stack.
     */

    /* FIQ mode stack */
    msr CPSR_c, #(MODE_FIQ | I_BIT | F_BIT)
    ldr sp, =(z_arm_fiq_stack + CONFIG_ARMV7_FIQ_STACK_SIZE)

    /* IRQ mode stack */
    msr CPSR_c, #(MODE_IRQ | I_BIT | F_BIT)
    ldr sp, =(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)

    /* ABT mode stack */
    msr CPSR_c, #(MODE_ABT | I_BIT | F_BIT)
    ldr sp, =(z_arm_abort_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE)

    /* UND mode stack */
    msr CPSR_c, #(MODE_UND | I_BIT | F_BIT)
    ldr sp, =(z_arm_undef_stack + CONFIG_ARMV7_EXCEPTION_STACK_SIZE)

    /* SVC mode stack */
    msr CPSR_c, #(MODE_SVC | I_BIT | F_BIT)
    ldr sp, =(z_arm_svc_stack + CONFIG_ARMV7_SVC_STACK_SIZE)

    /* SYS mode stack */
    msr CPSR_c, #(MODE_SYS | I_BIT | F_BIT)
    ldr sp, =(z_arm_sys_stack + CONFIG_ARMV7_SYS_STACK_SIZE)

#if defined(CONFIG_PLATFORM_SPECIFIC_INIT)
    /* Execute platform-specific initialisation if applicable */
    bl z_platform_init
#endif

#if defined(CONFIG_WDOG_INIT)
    /* board-specific watchdog initialization is necessary */
    bl z_arm_watchdog_init
#endif

    b z_arm_prep_c
