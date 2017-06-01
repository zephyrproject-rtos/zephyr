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

_ASM_FILE_PROLOGUE

GTEXT(__swap)
GTEXT(__svc)
GTEXT(__pendsv)
GTEXT(_do_kernel_oops)
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

#if defined (CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH) || \
		defined(CONFIG_STACK_SENTINEL)
    /* Register the context switch */
    push {lr}

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
    bl _sys_k_event_logger_context_switch
#endif
#ifdef CONFIG_STACK_SENTINEL
    bl _check_stack_sentinel
#endif
#if defined(CONFIG_ARMV6_M)
    pop {r0}
    mov lr, r0
#else
    pop {lr}
#endif /* CONFIG_ARMV6_M */
#endif /* CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH || CONFIG_STACK_SENTINEL */

    /* load _kernel into r1 and current k_thread into r2 */
    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]

    /* addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    /* save callee-saved + psp in thread */
    mrs ip, PSP

#if defined(CONFIG_ARMV6_M)
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
#elif defined(CONFIG_ARMV7_M)
    stmia r0, {v1-v8, ip}
#ifdef CONFIG_FP_SHARING
    add r0, r2, #_thread_offset_to_preempt_float
    vstmia r0, {s16-s31}
#endif /* CONFIG_FP_SHARING */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

    /*
     * Prepare to clear PendSV with interrupts unlocked, but
     * don't clear it yet. PendSV must not be cleared until
     * the new thread is context-switched in since all decisions
     * to pend PendSV have been taken with the current kernel
     * state and this is what we're handling currently.
     */
    ldr v4, =_SCS_ICSR
    ldr v3, =_SCS_ICSR_UNPENDSV

    /* protect the kernel state while we play with the thread lists */
#if defined(CONFIG_ARMV6_M)
    cpsid i
#elif defined(CONFIG_ARMV7_M)
    movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI, r0
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

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
    ldr r0, [r2, #_thread_offset_to_basepri]
    movs.n r3, #0
    str r3, [r2, #_thread_offset_to_basepri]

#if defined(CONFIG_ARMV6_M)
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
#elif defined(CONFIG_ARMV7_M)
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

    /* load callee-saved + psp from thread */
    add r0, r2, #_thread_offset_to_callee_saved
    ldmia r0, {v1-v8, ip}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

    msr PSP, ip

#ifdef CONFIG_EXECUTION_BENCHMARKING
#if defined(CONFIG_ARMV6_M)
    push {r3}
#endif
    push {lr}

    bl read_systick_end_of_swap

#if defined(CONFIG_ARMV6_M)
    pop {r3}
    mov lr,r3
    pop {r3}
#else
    pop {lr}
#endif
#endif

    /* exc return */
    bx lr

#if defined(CONFIG_ARMV6_M)
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

#elif defined(CONFIG_ARMV7_M)
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
    * 0: context switch
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * Planned implementation of system calls for memory protection will
    * expand this case.
    */
    ands r1, #0xff
    beq _context_switch

    cmp r1, #2
    beq _oops

#if CONFIG_IRQ_OFFLOAD
    push {lr}
    blx _irq_do_offload  /* call C routine which executes the offload */
    pop {lr}

    /* exception return is done in _IntExit() */
    b _IntExit
#endif

_context_switch:

    /*
     * Unlock interrupts:
     * - in a SVC call, so protected against context switches
     * - allow PendSV, since it's running at prio 0xff
     */
    eors.n r0, r0
    msr BASEPRI, r0

     /* set PENDSV bit, pending the PendSV exception */
    ldr r1, =_SCS_ICSR
    ldr r2, =_SCS_ICSR_PENDSV
    str r2, [r1, #0]

    /* handler mode exit, to PendSV */
    bx lr

_oops:
    push {lr}
    blx _do_kernel_oops
    pop {pc}

#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

/**
 *
 * @brief Initiate a cooperative context switch
 *
 * The __swap() routine is invoked by various kernel services to effect
 * a cooperative context context switch.  Prior to invoking __swap(), the caller
 * disables interrupts via irq_lock() and the return 'key' is passed as a
 * parameter to __swap().  The 'key' actually represents the BASEPRI register
 * prior to disabling interrupts via the BASEPRI mechanism.
 *
 * __swap() itself does not do much.
 *
 * It simply stores the intlock key (the BASEPRI value) parameter into
 * current->basepri, and then triggers a service call exception (svc) to setup
 * the PendSV exception, which does the heavy lifting of context switching.

 * This is the only place we have to save BASEPRI since the other paths to
 * __pendsv all come from handling an interrupt, which means we know the
 * interrupts were not locked: in that case the BASEPRI value is 0.
 *
 * Given that __swap() is called to effect a cooperative context switch,
 * only the caller-saved integer registers need to be saved in the thread of the
 * outgoing thread. This is all performed by the hardware, which stores it in
 * its exception stack frame, created when handling the svc exception.
 *
 * On Cortex-M0/M0+ the intlock key is represented by the PRIMASK register,
 * as BASEPRI is not available.
 *
 * @return may contain a return value setup by a call to
 * _set_thread_return_value()
 *
 * C function prototype:
 *
 * unsigned int __swap (unsigned int basepri);
 *
 */

SECTION_FUNC(TEXT, __swap)

#ifdef CONFIG_EXECUTION_BENCHMARKING
#if defined(CONFIG_ARMV6_M)
    push {r3}
#endif
    push {lr}
    bl read_systick_start_of_swap
#if defined(CONFIG_ARMV6_M)
    pop {r3}
    mov lr,r3
    pop {r3}
#else
    pop {lr}
#endif
#endif
    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]
    str r0, [r2, #_thread_offset_to_basepri]

    /*
     * Set __swap()'s default return code to -EAGAIN. This eliminates the need
     * for the timeout code to set it itself.
     */
    ldr r1, =_k_neg_eagain
    ldr r1, [r1]
    str r1, [r2, #_thread_offset_to_swap_return_value]

#if defined(CONFIG_ARMV6_M)
    /* No priority-based interrupt masking on M0/M0+,
     * pending PendSV is used instead of svc
     */
    ldr r1, =_SCS_ICSR
    ldr r3, =_SCS_ICSR_PENDSV
    str r3, [r1, #0]

    /* Unlock interrupts to allow PendSV, since it's running at prio 0xff
     *
     * PendSV handler will be called if there are no other interrupts
     * of a higher priority pending.
     */
    cpsie i
#elif defined(CONFIG_ARMV7_M)
    svc #0
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */

    /* coming back from exception, r2 still holds the pointer to _current */
    ldr r0, [r2, #_thread_offset_to_swap_return_value]
    bx lr
