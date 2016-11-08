/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Thread context switching for ARM Cortex-M
 *
 * This module implements the routines necessary for thread context switching
 * on ARM Cortex-M3/M4 CPUs.
 */

#define _ASMLANGUAGE

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(_Swap)
#if !defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
GTEXT(__svc)
#endif
GTEXT(__pendsv)
GTEXT(_get_next_ready_thread)
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

    _GDB_STUB_EXC_ENTRY

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
	/* Register the context switch */
	push {lr}
	bl _sys_k_event_logger_context_switch
	pop {r0}
	mov lr, r0
#endif

    /* load _kernel into r1 and current k_thread into r2 */
    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]

    /* addr of callee-saved regs in TCS in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    /* save callee-saved + psp in TCS */
    mrs ip, PSP

#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
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
#else
    stmia r0, {v1-v8, ip}
#ifdef CONFIG_FP_SHARING
    add r0, r2, #_thread_offset_to_preempt_float
    vstmia r0, {s16-s31}
#endif /* CONFIG_FP_SHARING */
#endif /* CONFIG_CPU_CORTEX_M0_M0PLUS */

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
#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
    cpsid i
#else /* CONFIG_CPU_CORTEX_M3_M4 */
    movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI, r0
#endif

    /* find out incoming thread (fiber or task) */

    mov.n v2, lr
    movs.n v1, r1
    blx _get_next_ready_thread
    movs.n r1, v1
    mov.n lr, v2
    movs.n r2, r0

    /* r2 contains the new thread */
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

#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
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
#else /* CONFIG_CPU_CORTEX_M3_M4 */
    /* restore BASEPRI for the incoming thread */
    msr BASEPRI, r0

#ifdef CONFIG_FP_SHARING
    add r0, r2, #_thread_offset_to_preempt_float
    vldmia r0, {s16-s31}
#endif

    /* load callee-saved + psp from TCS */
    add r0, r2, #_thread_offset_to_callee_saved
    ldmia r0, {v1-v8, ip}
#endif /* CONFIG_CPU_CORTEX_M0_M0PLUS */

    msr PSP, ip

    _GDB_STUB_EXC_EXIT

    /* exc return */
    bx lr

#if !defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is only used in _Swap() to enter handler mode so we
 * can go through the PendSV exception to perform a context switch.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT, __svc)

    _GDB_STUB_EXC_ENTRY

#if CONFIG_IRQ_OFFLOAD
    tst lr, #0x4    /* did we come from thread mode ? */
    ite eq  /* if zero (equal), came from handler mode */
        mrseq r0, MSP   /* handler mode, stack frame is on MSP */
        mrsne r0, PSP   /* thread mode, stack frame is on PSP */

    ldr r0, [r0, #24]   /* grab address of PC from stack frame */
    /* SVC is a two-byte instruction, point to it and read  encoding */
    ldrh r0, [r0, #-2]

   /*
    * grab service call number: if zero, it's a context switch;  if not,
    * it's an irq offload
    */
    ands r0, #0xff
    beq _context_switch

    push {lr}
    blx _irq_do_offload  /* call C routine which executes the offload */
    pop {lr}

    /* exception return is done in _IntExit(), including _GDB_STUB_EXC_EXIT */
    b _IntExit

_context_switch:
#endif

    /*
     * Set _Swap()'s default return code to -EAGAIN. This eliminates the
     * need for the timeout code to invoke fiberRtnValueSet().
     */

    mrs  r2, PSP   /* thread mode, stack frame is on PSP */
    ldr  r3, =_k_neg_eagain
    ldr  r3, [r3, #0]
    str  r3, [r2, #___esf_t_a1_OFFSET]

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

    _GDB_STUB_EXC_EXIT

    /* handler mode exit, to PendSV */
    bx lr
#endif /* !CONFIG_CPU_CORTEX_M0_M0PLUS */

/**
 *
 * @brief Initiate a cooperative context switch
 *
 * The _Swap() routine is invoked by various nanokernel services to effect
 * a cooperative context context switch.  Prior to invoking _Swap(), the caller
 * disables interrupts via irq_lock() and the return 'key' is passed as a
 * parameter to _Swap().  The 'key' actually represents the BASEPRI register
 * prior to disabling interrupts via the BASEPRI mechanism.
 *
 * _Swap() itself does not do much.
 *
 * It simply stores the intlock key (the BASEPRI value) parameter into
 * current->basepri, and then triggers a service call exception (svc) to setup
 * the PendSV exception, which does the heavy lifting of context switching.

 * This is the only place we have to save BASEPRI since the other paths to
 * __pendsv all come from handling an interrupt, which means we know the
 * interrupts were not locked: in that case the BASEPRI value is 0.
 *
 * Given that _Swap() is called to effect a cooperative context switch,
 * only the caller-saved integer registers need to be saved in the TCS of the
 * outgoing thread. This is all performed by the hardware, which stores it in
 * its exception stack frame, created when handling the svc exception.
 *
 * On Cortex-M0/M0+ the intlock key is represented by the PRIMASK register,
 * as BASEPRI is not available.
 *
 * @return may contain a return value setup by a call to fiberRtnValueSet()
 *
 * C function prototype:
 *
 * unsigned int _Swap (unsigned int basepri);
 *
 */

SECTION_FUNC(TEXT, _Swap)

    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]
    str r0, [r2, #_thread_offset_to_basepri]

#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
    /* No priority-based interrupt masking on M0/M0+,
     * pending PendSV is used instead of svc
     */
    ldr r1, =_SCS_ICSR
    ldr r2, =_SCS_ICSR_PENDSV
    str r2, [r1, #0]

    /* load -EAGAIN as the default return value */
    ldr r0, =_k_neg_eagain
    ldr r0, [r0]

    /* Unlock interrupts to allow PendSV, since it's running at prio 0xff
     *
     * PendSV handler will be called if there are no other interrupts
     * of a higher priority pending.
     */
    cpsie i

    /* PC stored in stack frame by the hw */
    bx lr
#else /* CONFIG_CPU_CORTEX_M3_M4 */
    svc #0

    /* r0 contains the return value if needed */
    bx lr
#endif
