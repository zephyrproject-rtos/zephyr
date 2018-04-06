/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <syscall.h>

_ASM_FILE_PROLOGUE

GTEXT(__svc)
GTEXT(__pendsv)
GTEXT(_do_kernel_oops)
GTEXT(_arm_do_syscall)
GDATA(_k_neg_eagain)

GDATA(_kernel)

/**
 *
 * @brief PendSV exception handler, handling context switches
 *
 * The PendSV exception is the only execution context in the system that can
 * perform context switching. When an execution context finds out it has to
 * switch contexts, it pends the PendSV exception.
 *
 * When PendSV is pended, the decision that a context switch must happen has
 * already been taken. In other words, when __pendsv() runs, we *know* we have
 * to swap *something*.
 */

SECTION_FUNC(TEXT, __pendsv)

#ifdef CONFIG_TRACING
    /* Register the context switch */
    push {lr}
    bl z_sys_trace_thread_switched_in
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0}
    mov lr, r0
#else
    pop {lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif /* CONFIG_TRACING */

    /* protect the kernel state while we play with the thread lists */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    cpsid i
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI, r0
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

    /* load _kernel into r1 and current k_thread into r2 */
    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]

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
    stmia r0, {v1-v8, ip}
#ifdef CONFIG_FP_SHARING
    add r0, r2, #_thread_offset_to_preempt_float
    vstmia r0, {s16-s31}
#endif /* CONFIG_FP_SHARING */
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
    ldr v4, =_SCS_ICSR
    ldr v3, =_SCS_ICSR_UNPENDSV

    /* _kernel is still in r1 */

    /* fetch the thread to run from the ready queue cache */
    ldr r2, [r1, _kernel_offset_to_ready_q_cache]

    str r2, [r1, #_kernel_offset_to_current]

    /*
     * Clear PendSV so that if another interrupt comes in and
     * decides, with the new kernel state baseed on the new thread
     * being context-switched in, that it needs to reschedules, it
     * will take, but that previously pended PendSVs do not take,
     * since they were based on the previous kernel state and this
     * has been handled.
     */

    /* _SCS_ICSR is still in v4 and _SCS_ICSR_UNPENDSV in v3 */
    str v3, [v4, #0]

    /* Restore previous interrupt disable state (irq_lock key) */
#if (defined(CONFIG_CPU_CORTEX_M0PLUS) || defined(CONFIG_CPU_CORTEX_M0)) && \
	    _thread_offset_to_basepri > 124
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
    movs.n r3, #0
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
    bne _thread_irq_disabled
    cpsie i
_thread_irq_disabled:

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

#ifdef CONFIG_FP_SHARING
    add r0, r2, #_thread_offset_to_preempt_float
    vldmia r0, {s16-s31}
#endif

#ifdef CONFIG_MPU_STACK_GUARD
    /* r2 contains k_thread */
    add r0, r2, #0
    push {r2, lr}
    blx configure_mpu_stack_guard
    pop {r2, lr}
#endif /* CONFIG_MPU_STACK_GUARD */

#ifdef CONFIG_BUILTIN_STACK_GUARD
    /* r2 contains k_thread */
    add r0, r2, #0
    push {r2, lr}
    blx configure_builtin_stack_guard
    pop {r2, lr}
#endif /* CONFIG_BUILTIN_STACK_GUARD */

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

    /* r2 contains k_thread */
    add r0, r2, #0
    push {r2, lr}
    blx configure_mpu_mem_domain
    pop {r2, lr}

    add r0, r2, #0
    push {r2, lr}
    blx configure_mpu_user_context
    pop {r2, lr}
#endif

    /* load callee-saved + psp from thread */
    add r0, r2, #_thread_offset_to_callee_saved
    ldmia r0, {v1-v8, ip}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

    msr PSP, ip

#ifdef CONFIG_EXECUTION_BENCHMARKING
    stm sp!,{r0-r3} /* Save regs r0 to r4 on stack */
    push {lr}
    bl read_timer_end_of_swap

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r3}
    mov lr,r3
#else
    pop {lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
    ldm sp!,{r0-r3} /* Load back regs ro to r4 */
#endif /* CONFIG_EXECUTION_BENCHMARKING */

    /* exc return */
    bx lr

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
SECTION_FUNC(TEXT, __svc)
    /* Use EXC_RETURN state to find out if stack frame is on the
     * MSP or PSP
     */
    ldr r0, =0x4
    mov r1, lr
    tst r1, r0
    beq _stack_frame_msp
    mrs r0, PSP
    bne _stack_frame_endif
_stack_frame_msp:
    mrs r0, MSP
_stack_frame_endif:

    /* Figure out what SVC call number was invoked */
    ldr r1, [r0, #24]   /* grab address of PC from stack frame */
    /* SVC is a two-byte instruction, point to it and read  encoding */
    subs r1, r1, #2
    ldrb r1, [r1, #0]

   /*
    * grab service call number:
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * Planned implementation of system calls for memory protection will
    * expand this case.
    */

    cmp r1, #2
    beq _oops

#if CONFIG_IRQ_OFFLOAD
    push {lr}
    blx _irq_do_offload  /* call C routine which executes the offload */
    pop {r3}
    mov lr, r3
#endif

    /* exception return is done in _IntExit() */
    b _IntExit

_oops:
    push {lr}
    blx _do_kernel_oops
    pop {pc}

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is only used in __swap() to enter handler mode so we
 * can go through the PendSV exception to perform a context switch.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, __svc)
    tst lr, #0x4    /* did we come from thread mode ? */
    ite eq  /* if zero (equal), came from handler mode */
        mrseq r0, MSP   /* handler mode, stack frame is on MSP */
        mrsne r0, PSP   /* thread mode, stack frame is on PSP */

    ldr r1, [r0, #24]   /* grab address of PC from stack frame */
    /* SVC is a two-byte instruction, point to it and read  encoding */
    ldrh r1, [r1, #-2]

   /*
    * grab service call number:
    * 0: Unused
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * 3: System call
    * Planned implementation of system calls for memory protection will
    * expand this case.
    */
    ands r1, #0xff
#if CONFIG_USERSPACE
    mrs r2, CONTROL

    cmp r1, #3
    beq _do_syscall

    /*
     * check that we are privileged before invoking other SVCs
     * oops if we are unprivileged
     */
    tst r2, #0x1
    bne _oops

#endif

    cmp r1, #2
    beq _oops

#if CONFIG_IRQ_OFFLOAD
    push {lr}
    blx _irq_do_offload  /* call C routine which executes the offload */
    pop {lr}

    /* exception return is done in _IntExit() */
    b _IntExit
#endif

_oops:
    push {lr}
    blx _do_kernel_oops
    pop {pc}

#if CONFIG_USERSPACE
    /*
     * System call will setup a jump to the _do_arm_syscall function
     * when the SVC returns via the bx lr.
     *
     * There is some trickery involved here because we have to preserve
     * the original LR value so that we can return back to the caller of
     * the SVC.
     *
     * On SVC exeption, the stack looks like the following:
     * r0 - r1 - r2 - r3 - r12 - LR - PC - PSR
     *
     * Registers look like:
     * r0 - arg1
     * r1 - arg2
     * r2 - arg3
     * r3 - arg4
     * r4 - arg5
     * r5 - arg6
     * r6 - call_id
     * r8 - saved link register
     */
_do_syscall:
    ldr r8, [r0, #24]   /* grab address of PC from stack frame */
    ldr r1, =_arm_do_syscall
    str r1, [r0, #24]   /* overwrite the LR to point to _arm_do_syscall */

    /* validate syscall limit, only set priv mode if valid */
    ldr ip, =K_SYSCALL_LIMIT
    cmp r6, ip
    blt valid_syscall_id

    /* bad syscall id.  Set arg0 to bad id and set call_id to SYSCALL_BAD */
    str r6, [r0, #0]
    ldr r6, =K_SYSCALL_BAD

valid_syscall_id:
    /* set mode to privileged, r2 still contains value from CONTROL */
    bic r2, #1
    msr CONTROL, r2

    /* ISB is not strictly necessary here (stack pointer is not being
     * touched), but it's recommended to avoid executing pre-fetched
     * instructions with the previous privilege.
     */
    isb

    /* return from SVC to the modified LR - _arm_do_syscall */
    bx lr
#endif

#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

