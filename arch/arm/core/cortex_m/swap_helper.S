/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARM Cortex-M
 *
 * This module implements the routines necessary for thread context switching
 * on ARM Cortex-M CPUs.
 */

#include <zephyr/toolchain.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_pendsv)

GDATA(_kernel)

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
GDATA(z_arm_tls_ptr)
#endif

/**
 *
 * @brief PendSV exception handler, handling context switches
 *
 * The PendSV exception is the only execution context in the system that can
 * perform context switching. When an execution context finds out it has to
 * switch contexts, it pends the PendSV exception.
 *
 * When PendSV is pended, the decision that a context switch must happen has
 * already been taken. In other words, when z_arm_pendsv() runs, we *know* we
 * have to swap *something*.
 *
 * For Cortex-M, z_arm_pendsv() is invoked with no arguments.
 */

SECTION_FUNC(TEXT, z_arm_pendsv)

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
    /* Register the context switch */
    push {r0, lr}
    bl z_thread_mark_switched_out
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r1}
    mov lr, r1
#else
    pop {r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif /* CONFIG_INSTRUMENT_THREAD_SWITCHING */

    /* load _kernel into r1 and current k_thread into r2 */
    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]

#if defined(CONFIG_ARM_STORE_EXC_RETURN)
    /* Store LSB of LR (EXC_RETURN) to the thread's 'mode' word. */
    strb lr, [r2, #_thread_offset_to_mode_exc_return]
#endif

    /* addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    /* save callee-saved + psp in thread */
    mrs ip, PSP

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    /* Store current r4-r7 */
    stmea r0!, {r4-r7}
    /* copy r8-r12 into r3-r7 */
    mov r3, r8
    mov r4, r9
    mov r5, r10
    mov r6, r11
    mov r7, ip
    /* store r8-12 */
    stmea r0!, {r3-r7}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    stmia r0, {r4-r11, ip}
#ifdef CONFIG_FPU_SHARING
    /* Assess whether switched-out thread had been using the FP registers. */
    tst lr, #_EXC_RETURN_FTYPE_Msk
    bne .L_out_fp_endif

    /* FP context active: set FP state and store callee-saved registers.
     * Note: if Lazy FP stacking is enabled, storing the callee-saved
     * registers will automatically trigger FP state preservation in
     * the thread's stack. This will also clear the FPCCR.LSPACT flag.
     */
    add r0, r2, #_thread_offset_to_preempt_float
    vstmia r0, {s16-s31}

.L_out_fp_endif:
    /* At this point FPCCR.LSPACT is guaranteed to be cleared,
     * regardless of whether the thread has an active FP context.
     */
#endif /* CONFIG_FPU_SHARING */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

    /* Protect the kernel state while we play with the thread lists */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    cpsid i
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI_MAX, r0
    isb /* Make the effect of disabling interrupts be realized immediately */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

    /*
     * Prepare to clear PendSV with interrupts unlocked, but
     * don't clear it yet. PendSV must not be cleared until
     * the new thread is context-switched in since all decisions
     * to pend PendSV have been taken with the current kernel
     * state and this is what we're handling currently.
     */
    ldr r7, =_SCS_ICSR
    ldr r6, =_SCS_ICSR_UNPENDSV

    /* _kernel is still in r1 */

    /* fetch the thread to run from the ready queue cache */
    ldr r2, [r1, #_kernel_offset_to_ready_q_cache]

    str r2, [r1, #_kernel_offset_to_current]

    /*
     * Clear PendSV so that if another interrupt comes in and
     * decides, with the new kernel state based on the new thread
     * being context-switched in, that it needs to reschedule, it
     * will take, but that previously pended PendSVs do not take,
     * since they were based on the previous kernel state and this
     * has been handled.
     */

    /* _SCS_ICSR is still in r7 and _SCS_ICSR_UNPENDSV in r6 */
    str r6, [r7, #0]

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
    /* Grab the TLS pointer */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    ldr r4, =_thread_offset_to_tls
    adds r4, r2, r4
    ldr r0, [r4]
#else
    ldr r0, [r2, #_thread_offset_to_tls]
#endif

    /* For Cortex-M, store TLS pointer in a global variable,
     * as it lacks the process ID or thread ID register
     * to be used by toolchain to access thread data.
     */
    ldr r4, =z_arm_tls_ptr
    str r0, [r4]
#endif

#if defined(CONFIG_ARM_STORE_EXC_RETURN)
    /* Restore EXC_RETURN value. */
    ldrsb lr, [r2, #_thread_offset_to_mode_exc_return]
#endif

    /* Restore previous interrupt disable state (irq_lock key)
     * (We clear the arch.basepri field after restoring state)
     */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) && (_thread_offset_to_basepri > 124)
    /* Doing it this way since the offset to thread->arch.basepri can in
     * some configurations be larger than the maximum of 124 for ldr/str
     * immediate offsets.
     */
    ldr r4, =_thread_offset_to_basepri
    adds r4, r2, r4

    ldr r0, [r4]
    movs.n r3, #0
    str r3, [r4]
#else
    ldr r0, [r2, #_thread_offset_to_basepri]
    movs r3, #0
    str r3, [r2, #_thread_offset_to_basepri]
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    /* BASEPRI not available, previous interrupt disable state
     * maps to PRIMASK.
     *
     * Only enable interrupts if value is 0, meaning interrupts
     * were enabled before irq_lock was called.
     */
    cmp r0, #0
    bne .L_thread_irq_disabled
    cpsie i
.L_thread_irq_disabled:

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
    /* Re-program dynamic memory map */
    push {r2,lr}
    mov r0, r2
    bl z_arm_configure_dynamic_mpu_regions
    pop {r2,r3}
    mov lr, r3
#endif

#ifdef CONFIG_USERSPACE
    /* restore mode */
    ldr r3, =_thread_offset_to_mode
    adds r3, r2, r3
    ldr r0, [r3]
    mrs r3, CONTROL
    movs.n r1, #1
    bics r3, r1
    orrs r3, r0
    msr CONTROL, r3

    /* ISB is not strictly necessary here (stack pointer is not being
     * touched), but it's recommended to avoid executing pre-fetched
     * instructions with the previous privilege.
     */
    isb

#endif

    ldr r4, =_thread_offset_to_callee_saved
    adds r0, r2, r4

    /* restore r4-r12 for new thread */
    /* first restore r8-r12 located after r4-r7 (4*4bytes) */
    adds r0, #16
    ldmia r0!, {r3-r7}
    /* move to correct registers */
    mov r8, r3
    mov r9, r4
    mov r10, r5
    mov r11, r6
    mov ip, r7
    /* restore r4-r7, go back 9*4 bytes to the start of the stored block */
    subs r0, #36
    ldmia r0!, {r4-r7}
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* restore BASEPRI for the incoming thread */
    msr BASEPRI, r0

#ifdef CONFIG_FPU_SHARING
    /* Assess whether switched-in thread had been using the FP registers. */
    tst lr, #_EXC_RETURN_FTYPE_Msk
    beq .L_in_fp_active
    /* FP context inactive for swapped-in thread:
     * - reset FPSCR to 0
     * - set EXC_RETURN.F_Type (prevents FP frame un-stacking when returning
     *   from pendSV)
     */
    movs.n r3, #0
    vmsr fpscr, r3
    b .L_in_fp_endif

.L_in_fp_active:
    /* FP context active:
     * - clear EXC_RETURN.F_Type
     * - FPSCR and caller-saved registers will be restored automatically
     * - restore callee-saved FP registers
     */
    add r0, r2, #_thread_offset_to_preempt_float
    vldmia r0, {s16-s31}
.L_in_fp_endif:
    /* Clear CONTROL.FPCA that may have been set by FP instructions */
    mrs r3, CONTROL
    bic r3, #_CONTROL_FPCA_Msk
    msr CONTROL, r3
    isb
#endif

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
    /* Re-program dynamic memory map */
    push {r2,lr}
    mov r0, r2 /* _current thread */
    bl z_arm_configure_dynamic_mpu_regions
    pop {r2,lr}
#endif

#ifdef CONFIG_USERSPACE
    /* restore mode */
    ldr r0, [r2, #_thread_offset_to_mode]
    mrs r3, CONTROL
    bic r3, #1
    orr r3, r0
    msr CONTROL, r3

    /* ISB is not strictly necessary here (stack pointer is not being
     * touched), but it's recommended to avoid executing pre-fetched
     * instructions with the previous privilege.
     */
    isb

#endif

    /* load callee-saved + psp from thread */
    add r0, r2, #_thread_offset_to_callee_saved
    ldmia r0, {r4-r11, ip}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

    msr PSP, ip

#ifdef CONFIG_BUILTIN_STACK_GUARD
    /* r2 contains k_thread */
    add r0, r2, #0
    push {r2, lr}
    bl configure_builtin_stack_guard
    pop {r2, lr}
#endif /* CONFIG_BUILTIN_STACK_GUARD */

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
    /* Register the context switch */
    push {r0, lr}
    bl z_thread_mark_switched_in
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r1}
    mov lr, r1
#else
    pop {r0, lr}
#endif
#endif /* CONFIG_INSTRUMENT_THREAD_SWITCHING */

    /*
     * Cortex-M: return from PendSV exception
     */
    bx lr
