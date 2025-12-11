/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
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
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include <zephyr/kernel.h>

#include <zephyr/arch/arm/cortex_m/cpu.h>
_ASM_FILE_PROLOGUE

GTEXT(z_arm_svc)
GTEXT(z_arm_pendsv)
GTEXT(z_do_kernel_oops)
#if defined(CONFIG_USERSPACE)
GTEXT(z_arm_do_syscall)
#endif

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

#ifdef CONFIG_ARM_PAC_PER_THREAD
    /* Read thread's dedicated PAC key and write them in the privileged PAC key registers.
     * Note: There is no way to know the _current thread mode after the thread creation since
     * `_current->arch.mode` for the userspace thread is set in z_arm_userspace_enter() which
     * runs after z_arm_pendsv is done. So for now, while switching to an unprivileged thread
     * the same PAC keys are set in both privileged and unprivileged PAC key registers.
     * TODO: find a way to not set privileged PAC keys if the thread being switched into is an
     * unprivileged thread.
     */
    add r0, r2, #_thread_offset_to_pac_keys
    ldmia r0!, {r3-r6}
    msr  PAC_KEY_P_0, r3
    msr  PAC_KEY_P_1, r4
    msr  PAC_KEY_P_2, r5
    msr  PAC_KEY_P_3, r6
    clrm {r3-r6}
#endif /* CONFIG_ARM_PAC_PER_THREAD */

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

/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is used in the following occasions:
 * - IRQ offloading
 * - Kernel run-time exceptions
 * - System Calls (User mode)
 *
 */
SECTION_FUNC(TEXT, z_arm_svc)
  /* Use EXC_RETURN state to find out if stack frame is on the
   * MSP or PSP
   */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
  movs r0, #_EXC_RETURN_SPSEL_Msk
  mov r1, lr
  tst r1, r0
  beq .L_stack_frame_msp
  mrs r0, PSP
  bne .L_stack_frame_endif
.L_stack_frame_msp:
  mrs r0, MSP
.L_stack_frame_endif:
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    tst lr, #_EXC_RETURN_SPSEL_Msk /* did we come from thread mode ? */
    ite eq  /* if zero (equal), came from handler mode */
        mrseq r0, MSP   /* handler mode, stack frame is on MSP */
        mrsne r0, PSP   /* thread mode, stack frame is on PSP */
#endif


    /* Figure out what SVC call number was invoked */

    ldr r1, [r0, #24]   /* grab address of PC from stack frame */
    /* SVC is a two-byte instruction, point to it and read the
     * SVC number (lower byte of SCV instruction)
     */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    subs r1, r1, #2
    ldrb r1, [r1]
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldrb r1, [r1, #-2]
#endif

   /*
    * grab service call number:
    * 0: Unused
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * 3: System call (if user mode supported)
    */
#if defined(CONFIG_USERSPACE)
    mrs r2, CONTROL

    cmp r1, #3
    beq .L_do_syscall

    /*
     * check that we are privileged before invoking other SVCs
     * oops if we are unprivileged
     */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    movs r3, #0x1
    tst r2, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    tst r2, #0x1
#endif
    bne .L_oops

#endif /* CONFIG_USERSPACE */

    cmp r1, #2
    beq .L_oops

#if defined(CONFIG_IRQ_OFFLOAD)
    push {r0, lr}
    bl z_irq_do_offload  /* call C routine which executes the offload */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r3}
    mov lr, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    pop {r0, lr}
#endif

    /* exception return is done in z_arm_int_exit() */
    ldr r0, =z_arm_int_exit
    bx r0

#endif

.L_oops:
    push {r0, lr}
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* Build _callee_saved_t. To match the struct
     * definition we push the psp & then r11-r4
     */
    mrs r1, PSP
    push {r1, r2}
    push {r4-r11}
    mov  r1, sp /* pointer to _callee_saved_t */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
    mov r2, lr /* EXC_RETURN */
    bl z_do_kernel_oops
    /* return from SVC exception is done here */
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* We do not need to restore any register state here
     * because we did not use any callee-saved registers
     * in this routine. Therefore, we can just reset
     * the MSP to its value prior to entering the function
     */
    add sp, #40
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */
    pop {r0, pc}

#if defined(CONFIG_USERSPACE)
    /*
     * System call will setup a jump to the z_arm_do_syscall() function
     * when the SVC returns via the bx lr.
     *
     * There is some trickery involved here because we have to preserve
     * the original PC value so that we can return back to the caller of
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
.L_do_syscall:
/*
 * Build a privilege stack frame from the user stack frame, then switch PSP
 * to it. This ensures return from SVC does not rely on the user stack.
 *
 * Layout of privilege stack created from user stack:
 *
 *  +------+-------------------------+------+-------------------------+--------------------------+
 *  |  User stack                    |  Privilege stack               | Notes                    |
 *  +------+-------------------------+------+-------------------------+--------------------------+
 *  |Offset| contents                |Offset| contents                |                          |
 *  +------+-------------------------+------+-------------------------+--------------------------+
 *  |  0   | R0                   -> | 0    | R0                      | PSP switches from 0th    |
 *  |      |                         |      |                         | offset of user stack to  |
 *  |      |                         |      |                         | 0th offset of priv stack |
 *  |  4   | R1                   -> | 4    | R1                      |                          |
 *  |  8   | R2                   -> | 8    | R2                      |                          |
 *  | 12   | R3                   -> |12    | R3                      |                          |
 *  | 16   | R12                  -> |16    | R12                     |                          |
 *  | 20   | LR                   -> |20    | LR                      |                          |
 *  | 24   | Return Address      -x> |24    | z_arm_do_syscall        |return address from user  |
 *  |      |                         |      |                         |sf is not copied. Instead,|
 *  |      |                         |      |                         |it is replaced so that    |
 *  |      |                         |      |                         |z_arm_svc returns to      |
 *  |      |                         |      |                         |z_arm_do_syscall.         |
 *  |      |                         |      |                         |                          |
 *  | 28   | xPSR (w/ or w/o pad) -> |28    | xPSR (pad bit cleared)  |This completes the basic  |
 *  |      |                         |      |                         |exception sf w/ or w/o pad|
 *  |      |                         |      |                         |                          |
 *  | --   | FP regs + FPSCR      -> |--    | FP regs + FPSCR         |For arch supporting fp    |
 *  |      | (w/ or w/o pad)         |      |                         |context an additional     |
 *  |      |                         |      |                         |extended sf is copied.    |
 *  |________________________________|______|_________________________|__________________________|
 *  |      |                         |      |                         |On returning to           |
 *  |      |                         |      |                         |z_arm_do_syscall, the     |
 *  |      |                         |      |                         |above sf has already been |
 *  |      |                         |      |                         |unstacked and 8B from the |
 *  |      |                         |      |                         |then sf are used to pass  |
 *  |      |                         |      |                         |original pre-svc sp & the |
 *  |      |                         |      |                         |return address.           |
 *  |      |                         |      |                         |Note: at the moment       |
 *  |      |                         |      |                         |z_arm_do_syscall also     |
 *  |      |                         |      |                         |expects the return address|
 *  |      |                         |      |                         |to be set in r8.          |
 *  |      |                         |      |                         |                          |
 *  |      |                         | 0    | address that            |z_arm_do_syscall expects  |
 *  |      |                         |      | z_arm_do_syscall should |the original pre-svc sp at|
 *  |      |                         |      | set as PSP before       |0th offset i.e. new sp[0] |
 *  |      |                         |      | returning from svc.     |and,                      |
 *  |      |                         |      |                         |                          |
 *  |      |                         | 4    | Address that            |the return address at     |
 *  |      |                         |      | z_arm_do_syscall should |sp[4]. Note that this is  |
 *  |      |                         |      | return to after handling|the return address copied |
 *  |      |                         |      | svc                     |from user exception sf[24]|
 *  |      |                         |      |                         |which was not copied in   |
 *  |      |                         |      |                         |the previous sf.          |
 *  +------+-------------------------+------+-------------------------+--------------------------+
 * "sf" in this function is used as abbreviation for "stack frame".
 * Note that the "FP regs + FPSCR" are only present if CONFIG_FPU_SHARING=y, and the optional pad
 * is only present if PSP was not 8-byte aligned when SVC was executed.
 * Also note that FPU cannot be present in ARMv6-M or ARMv8-M Baseline implementations
 * (i.e., it may only be present when CONFIG_ARMV7_M_ARMV8_M_MAINLINE is enabled).
 */
 /* Start by fetching the top of privileged stack */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    ldr r1, =_kernel
    ldr r1, [r1, #_kernel_offset_to_current]
    adds r1, r1, #_thread_offset_to_priv_stack_start
    ldr r1, [r1] /* bottom of priv stack */
    ldr r3, =CONFIG_PRIVILEGED_STACK_SIZE
    subs r3, #(_EXC_HW_SAVED_BASIC_SF_SIZE+8) /* 8 for original sp and pc */
    add r1, r3
    mov ip, r1
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldr ip, =_kernel
    ldr ip, [ip, #_kernel_offset_to_current]
    ldr ip, [ip, #_thread_offset_to_priv_stack_start] /* bottom of priv stack */
    add ip, #CONFIG_PRIVILEGED_STACK_SIZE
#ifdef CONFIG_FPU_SHARING
    /* Assess whether svc calling thread had been using the FP registers. */
    tst lr, #_EXC_RETURN_FTYPE_Msk
    ite eq
    moveq r8, #_EXC_HW_SAVED_EXTENDED_SF_SIZE
    movne r8, #_EXC_HW_SAVED_BASIC_SF_SIZE
#else
    mov r8, #_EXC_HW_SAVED_BASIC_SF_SIZE
#endif
    sub ip, #8 /* z_arm_do_syscall will use this to get original sp and pc */
    sub ip, r8 /* 32 for basic sf + 72 for the optional esf */
#endif

    /*
     * At this point:
     * r0 has PSP i.e. top of user stack
     * ip has top of privilege stack
     * r8 has hardware-saved stack frame size (only in case of mainline)
     */
    push {r4-r7}
    push {r2}
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    mov  r2, r0 /* safe to use r2 since it is saved on MSP */

    /* Check for padding in the sf */
    ldr  r1, [r0, #_EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET] /* grab xPSR from sf which has the pad bit */
    movs r3, #1
    /* Check if pad bit 9 is set */
    lsls r3, r3, #9
    tst  r1, r3
    beq  .L_no_padding
    /* special handling for padded sf */
    bics r1, r3 /* clear the pad bit (priv stack is aligned and doesn't need it) */
    adds r2, #4
.L_no_padding:
    /* Calculate original pre-svc user sp which is psp + sf size (+4B if pad bit was set) */
    adds r2, #_EXC_HW_SAVED_BASIC_SF_SIZE
    mov  r3, ip
    str r2,[r3, #0]

    /* Store the pre-SVC user SP at the offset expected by z_arm_do_syscall,
     * as detailed in the table above.
     */
    str r2,[r3, #_EXC_HW_SAVED_BASIC_SF_SIZE]
    /* sf of priv stack has the same xPSR as user stack but with 9th bit reset */
    str r1,[r3, #_EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET]

    /* r0-r3, r12, LR from user stack sf are copied to sf of priv stack */
    mov r1, r0
    mov r2, r3
    ldmia r1!, {r4-r7}
    stmia r2!, {r4-r7}
    ldmia r1!, {r4-r5}
    stmia r2!, {r4-r5}

    /* Store the svc return address at the offset expected by z_arm_do_syscall,
     * as detailed in the table above.
     */
    str r5, [r3, #(_EXC_HW_SAVED_BASIC_SF_SIZE+4)]

    ldr r1, =z_arm_do_syscall
    str r1, [r3, #_EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET] /* Execution return to z_arm_do_syscall */
    ldr r1, [r0, #_EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET] /* grab address of PC from stack frame */
    /* Store the svc return address (i.e. next instr to svc) in r8 as expected by z_arm_do_syscall.
     */
    mov r8, r1

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    mov  r2, r0 /* safe to use r2 since it is saved on MSP */

    /* Calculate original pre-svc user sp without pad which is psp + sf size */
    add r2, r8

    /* Also, check for padding in the sf */
    ldr  r1, [r0, #_EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET] /* grab xPSR from sf which has the pad bit */
    tst r1, #(1<<9) /* Check if pad bit 9 is set */
    beq .L_no_padding
    bics r1, #(1<<9) /* clear the pad bit (priv stack is aligned and doesn't need it) */
    /* Calculate original pre-svc user sp with pad */
    add  r2, #4
.L_no_padding:
    str r2,[ip, #0]
    /* Store the pre-SVC user SP at the offset expected by z_arm_do_syscall,
     * as detailed in the table above.
     */
    str r2,[ip, r8]
    str r1,[ip, #_EXC_HW_SAVED_BASIC_SF_XPSR_OFFSET] /* priv sf get user sf xPSR with bit9 reset */

    /* r0-r3, r12, LR from user stack sf are copied to sf of priv stack */
    mov r1, r0
    mov r2, ip
    ldmia r1!, {r4-r7}
    stmia r2!, {r4-r7}
    ldmia r1!, {r4-r5}
    stmia r2!, {r4-r5}

    /* Store the svc return address at the offset expected by z_arm_do_syscall,
     * as detailed in the table above.
     */
    add r8, #4
    str r5, [ip, r8]

    ldr r1, =z_arm_do_syscall
    str r1, [ip, #_EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET] /* Execution return to z_arm_do_syscall */
    ldr r1, [r0, #_EXC_HW_SAVED_BASIC_SF_RETADDR_OFFSET] /* grab address of PC from stack frame */
    /* Store the svc return address (i.e. next instr to svc) in r8 as expected by z_arm_do_syscall.
     */
    mov r8, r1

    /* basic stack frame is copied at this point to privilege stack,
     * now time to copy the fp context
     */
#ifdef CONFIG_FPU_SHARING
    tst lr, #_EXC_RETURN_FTYPE_Msk
    bne .L_skip_fp_copy
    add r1, r0, #32
    add r2, ip, #32

    vldmia r1!, {s0-s15}
    vstmia r2!, {s0-s15}

    /* copy FPSCR + reserved (8 bytes) */
    ldmia   r1!, {r4, r5}
    stmia   r2!, {r4, r5}
.L_skip_fp_copy:
#endif

#endif
    pop {r2} /* restore CONTROL value */
    pop {r4-r7}

    /* Point PSP to privilege stack,
     * note that r0 still has the old PSP
     */
    msr PSP, ip

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    ldr r3, =K_SYSCALL_LIMIT
    cmp r6, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* validate syscall limit */
    ldr ip, =K_SYSCALL_LIMIT
    cmp r6, ip
#endif
    /* The supplied syscall_id must be lower than the limit
     * (Requires unsigned integer comparison)
     */
    blo .L_valid_syscall_id

    /* bad syscall id.  Set arg1 to bad id and set call_id to SYSCALL_BAD */
    str r6, [r0]
    ldr r6, =K_SYSCALL_BAD

    /* Bad syscalls treated as valid syscalls with ID K_SYSCALL_BAD. */

.L_valid_syscall_id:
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_current]
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    mov ip, r2
    ldr r1, =_thread_offset_to_mode
    ldr r3, [r0, r1]
    movs r2, #1
    bics r3, r2
    /* Store (privileged) mode in thread's mode state variable */
    str r3, [r0, r1]
    mov r2, ip
    dsb
    /* set mode to privileged, r2 still contains value from CONTROL */
    movs r3, #1
    bics r2, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldr r1, [r0, #_thread_offset_to_mode]
    bic r1, #1
    /* Store (privileged) mode in thread's mode state variable */
    str r1, [r0, #_thread_offset_to_mode]
    dsb
    /* set mode to privileged, r2 still contains value from CONTROL */
    bic r2, #1
#endif
    msr CONTROL, r2

    /* ISB is not strictly necessary here (stack pointer is not being
     * touched), but it's recommended to avoid executing pre-fetched
     * instructions with the previous privilege.
     */
    isb

#if defined(CONFIG_BUILTIN_STACK_GUARD)
    /* Set stack pointer limit (needed in privileged mode) */
    ldr ip, =_kernel
    ldr ip, [ip, #_kernel_offset_to_current]
    ldr ip, [ip, #_thread_offset_to_priv_stack_start]    /* priv stack ptr */
    msr PSPLIM, ip
#endif

    /* return from SVC to the modified LR - z_arm_do_syscall */
    bx lr
#endif /* CONFIG_USERSPACE */
