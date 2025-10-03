/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
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
#include "vector_table.h"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_reset)
GTEXT(arch_early_memset)
GDATA(z_interrupt_stacks)
GDATA(z_main_stack)
#if defined(CONFIG_DEBUG_THREAD_INFO)
GDATA(z_sys_post_kernel)
#endif
#if defined(CONFIG_SOC_RESET_HOOK)
GTEXT(soc_reset_hook)
#endif
#if defined(CONFIG_INIT_ARCH_HW_AT_BOOT)
GTEXT(z_arm_init_arch_hw_at_boot)
#endif
#if defined(CONFIG_PM_S2RAM)
GTEXT(arch_pm_s2ram_resume)
#endif

/*
 * PACBTI Mask for CONTROL register:
 * bit 4 - BTI_EN, bit 5 - UBTI_EN,
 * bit 6 - PAC_EN, bit 7 - UPAC_EN
 */
#ifdef CONFIG_ARM_PAC
#define CONTROL_ARM_PAC_MASK	(1<<6)|(1<<7)
#else
#define CONTROL_ARM_PAC_MASK	0
#endif

#ifdef CONFIG_ARM_BTI
#define CONTROL_ARM_BTI_MASK	(1<<4)|(1<<5)
#else
#define CONTROL_ARM_BTI_MASK	0
#endif

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset, or when the firmware image is chain-
 * loaded by another application (for instance, a bootloader). At minimum, the
 * processor must be in thread mode with privileged level. At this point, the
 * main stack pointer (MSP) should be already pointing to a valid area in SRAM.
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
 * When these steps are completed, jump to z_prep_c(), which will finish
 * setting up the system for running C code.
 *
 */

SECTION_SUBSEC_FUNC(TEXT,_reset_section,z_arm_reset)

/*
 * The entry point is located at the z_arm_reset symbol, which
 * is fetched by a XIP image playing the role of a bootloader, which jumps to
 * it, not through the reset vector mechanism. Such bootloaders might want to
 * search for a __start symbol instead, so create that alias here.
 */
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__start)

#if defined(CONFIG_INIT_ARCH_HW_AT_BOOT)
    /* Reset CONTROL register */
    movs.n r0, #0
    msr CONTROL, r0
    isb
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
    /* Clear SPLIM registers */
    movs.n r0, #0
    msr MSPLIM, r0
    msr PSPLIM, r0
#endif /* CONFIG_CPU_CORTEX_M_HAS_SPLIM */

#endif /* CONFIG_INIT_ARCH_HW_AT_BOOT */

#if defined(CONFIG_PM_S2RAM)
    /*
     * Temporarily set MSP to interrupt stack so that arch_pm_s2ram_resume can
     * use stack for calling pm_s2ram_mark_check_and_clear.
     * This is safe because suspend never being called from an ISR ensures that
     * interrupt stack was not live during suspend.
     * Note:
     * if resuming from suspend, MSP is restored from cpu context
     * if not resuming, MSP will be set back to z_main_stack for proper init
     * And, apart from arch_pm_s2ram_resume being able to use the stack for
     * a short while, there is no change in behavior in either of the paths.
     */
    ldr r0, =z_interrupt_stacks + CONFIG_ISR_STACK_SIZE + MPU_GUARD_ALIGN_AND_SIZE
    msr msp, r0

    bl arch_pm_s2ram_resume
#endif /* CONFIG_PM_S2RAM */

    /*
     * MSP needs to be set to main stack for following scenarios:
     * - If CONFIG_PM_S2RAM is enabled, MSP is set to z_interrupt_stacks
     *   for resume functions to make use of a stack. However,
     *   if the device was not suspended and if resume functions return,
     *   MSP needs to be set back to z_main_stack to ensure proper
     *   initialization.
     * - If CONFIG_PM_S2RAM is not enabled but CONFIG_INIT_ARCH_HW_AT_BOOT is,
     *   MSP needs to be set to z_main_stack for proper initialization in case
     *   device was loaded through chain loading or a debugger, as the initial
     *   value of MSP would be anything that the previous image loaded.
     * - If CONFIG_INIT_STACKS is enabled, we need to ensure MSP is not set
     *   to z_interrupt_stacks, so we set it to z_main_stack.
     *
     * Since these scenarios cover most of the cases, we set MSP to
     * z_main_stack here.
     *
     */
    ldr r0, =z_main_stack + CONFIG_MAIN_STACK_SIZE
    msr msp, r0

    /* Note: Make sure that variables like z_sys_post_kernel
     *       are set after the call to arch_pm_s2ram_resume
     *       to avoid any issues with suspend/resume path.
     *       Refer issue #83660 for more details.
     */
#if defined(CONFIG_DEBUG_THREAD_INFO)
    /* Clear z_sys_post_kernel flag for RTOS aware debuggers */
    movs.n r0, #0
    ldr r1, =z_sys_post_kernel
    strb r0, [r1]
#endif /* CONFIG_DEBUG_THREAD_INFO */

#if defined(CONFIG_SOC_RESET_HOOK)
    bl soc_reset_hook
#endif

#if defined(CONFIG_INIT_ARCH_HW_AT_BOOT)
#if defined(CONFIG_CPU_HAS_ARM_MPU)
    /* Disable MPU */
    movs.n r0, #0
    ldr r1, =_SCS_MPU_CTRL
    str r0, [r1]
    dsb
#endif /* CONFIG_CPU_HAS_ARM_MPU */

    /* Initialize core architecture registers and system blocks */
    bl z_arm_init_arch_hw_at_boot
#endif /* CONFIG_INIT_ARCH_HW_AT_BOOT */

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

/*
 *
 * Note: in all Cortex-M variants the interrupt stack area is at
 * least equal to CONFIG_ISR_STACK_SIZE + MPU_GUARD_ALIGN_AND_SIZE
 * (may be larger due to rounding up for stack pointer aligning
 * purposes but this is sufficient during initialization).
 */

#ifdef CONFIG_INIT_STACKS
    ldr r0, =z_interrupt_stacks
    ldr r1, =0xaa
    ldr r2, =CONFIG_ISR_STACK_SIZE + MPU_GUARD_ALIGN_AND_SIZE
    bl arch_early_memset
#endif

    /*
     * Set PSP and use it to boot without using MSP, so that it
     * gets set to z_interrupt_stacks during initialization.
     */
    ldr r0, =z_interrupt_stacks
    ldr r1, =CONFIG_ISR_STACK_SIZE + MPU_GUARD_ALIGN_AND_SIZE
    adds r0, r0, r1
    msr PSP, r0
    mrs r0, CONTROL
    movs r1, #(2 | CONTROL_ARM_PAC_MASK | CONTROL_ARM_BTI_MASK)
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
     * supported on all platforms. So it is used when jumping to z_prep_c
     * (even though we do not intend to return).
     */
    bl z_prep_c
