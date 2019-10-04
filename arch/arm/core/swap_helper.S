/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARM Cortex-M and Cortex-R
 *
 * This module implements the routines necessary for thread context switching
 * on ARM Cortex-M and Cortex-R CPUs.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <syscall.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_svc)
GTEXT(z_arm_pendsv)
GTEXT(z_do_kernel_oops)
GTEXT(z_arm_do_syscall)
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
 * already been taken. In other words, when z_arm_pendsv() runs, we *know* we
 * have to swap *something*.
 *
 * For Cortex-M, z_arm_pendsv() is invoked with no arguments.
 *
 * For Cortex-R, PendSV exception is not supported by the architecture and this
 * function is directly called either by _IntExit in case of preemption, or
 * z_arm_svc in case of cooperative switching.
 */

SECTION_FUNC(TEXT, z_arm_pendsv)

#ifdef CONFIG_TRACING
    /* Register the context switch */
    push {r0, lr}
    bl sys_trace_thread_switched_out
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r1}
    mov lr, r1
#else
    pop {r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#endif /* CONFIG_TRACING */

    /* load _kernel into r1 and current k_thread into r2 */
    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]

    /* addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    /* save callee-saved + psp in thread */
#if defined(CONFIG_CPU_CORTEX_M)
    mrs ip, PSP
#endif

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
    /* Assess whether switched-out thread had been using the FP registers. */
    ldr r0, =0x10 /* EXC_RETURN.F_Type Mask */
    tst lr, r0    /* EXC_RETURN & EXC_RETURN.F_Type_Msk */
    beq out_fp_active
    /* FP context inactive: clear FP state */
    ldr r0, [r2, #_thread_offset_to_mode]
    bic r0, #0x4 /* _current->arch.mode &= ~(CONTROL_FPCA_Msk) */
    b out_fp_endif

out_fp_active:
    /* FP context active: set FP state and store callee-saved registers */
    add r0, r2, #_thread_offset_to_preempt_float
    vstmia r0, {s16-s31}
    ldr r0, [r2, #_thread_offset_to_mode]
    orrs r0, r0, #0x4 /* _current->arch.mode |= CONTROL_FPCA_Msk */

out_fp_endif:
    str r0, [r2, #_thread_offset_to_mode]
#endif /* CONFIG_FP_SHARING */
#elif defined(CONFIG_ARMV7_R)
    /* Store rest of process context */
    mrs r12, SPSR
    stm r0, {r4-r12,sp,lr}^
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

    /* Protect the kernel state while we play with the thread lists */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    cpsid i
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI, r0
    isb /* Make the effect of disabling interrupts be realized immediately */
#elif defined(CONFIG_ARMV7_R)
    /*
     * Interrupts are still disabled from z_arch_swap so empty clause
     * here to avoid the preprocessor error below
     */
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
#if defined(CONFIG_CPU_CORTEX_M)
    ldr v4, =_SCS_ICSR
    ldr v3, =_SCS_ICSR_UNPENDSV
#endif

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

    /* _SCS_ICSR is still in v4 and _SCS_ICSR_UNPENDSV in v3 */
#if defined(CONFIG_CPU_CORTEX_M)
    str v3, [v4, #0]
#endif

    /* Restore previous interrupt disable state (irq_lock key)
     * (We clear the arch.basepri field after restoring state)
     */
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
    bne _thread_irq_disabled
    cpsie i
_thread_irq_disabled:

#if defined(CONFIG_ARM_MPU)
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

#ifdef CONFIG_FP_SHARING
    /* Assess whether switched-in thread had been using the FP registers. */
    ldr r0, [r2, #_thread_offset_to_mode]
    tst r0, #0x04 /* thread.arch.mode & CONTROL.FPCA Msk */
    bne in_fp_active
    /* FP context inactive for swapped-in thread:
     * - reset FPSCR to 0
     * - set EXC_RETURN.F_Type (prevents FP frame un-stacking when returning
     *   from pendSV)
     */
    movs.n r3, #0
    vmsr fpscr, r3
    orrs lr, lr, #0x10 /* EXC_RETURN & EXC_RETURN.F_Type_Msk */
    b in_fp_endif

in_fp_active:
    /* FP context active:
     * - clear EXC_RETURN.F_Type
     * - FPSCR and caller-saved registers will be restored automatically
     * - restore callee-saved FP registers
     */
    bic lr, #0x10 /* EXC_RETURN | (~EXC_RETURN.F_Type_Msk) */
    add r0, r2, #_thread_offset_to_preempt_float
    vldmia r0, {s16-s31}
in_fp_endif:
    /* Clear CONTROL.FPCA that may have been set by FP instructions */
    mrs r3, CONTROL
    bic r3, #0x4 /* CONTROL.FPCA Msk */
    msr CONTROL, r3
    isb
#endif

#if defined (CONFIG_ARM_MPU)
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
    ldmia r0, {v1-v8, ip}
#elif defined(CONFIG_ARMV7_R)
_thread_irq_disabled:
    /* load _kernel into r1 and current k_thread into r2 */
    ldr r1, =_kernel
    ldr r2, [r1, #_kernel_offset_to_current]

    /* addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    /* restore r4-r12 for incoming thread, plus system sp and lr */
    ldm r0, {r4-r12,sp,lr}^
    msr SPSR_fsxc, r12
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

#ifdef CONFIG_BUILTIN_STACK_GUARD
    /* clear stack pointer limit before setting the PSP */
    mov r0, #0
    msr PSPLIM, r0
#endif /* CONFIG_BUILTIN_STACK_GUARD */

#if defined(CONFIG_CPU_CORTEX_M)
    msr PSP, ip
#endif

#ifdef CONFIG_BUILTIN_STACK_GUARD
    /* r2 contains k_thread */
    add r0, r2, #0
    push {r2, lr}
    bl configure_builtin_stack_guard
    pop {r2, lr}
#endif /* CONFIG_BUILTIN_STACK_GUARD */

#ifdef CONFIG_EXECUTION_BENCHMARKING
    push {r0, lr}
    bl read_timer_end_of_swap
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r1}
    mov lr,r1
#else
    pop {r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

#endif /* CONFIG_EXECUTION_BENCHMARKING */

#ifdef CONFIG_TRACING
    /* Register the context switch */
    push {r0, lr}
    bl sys_trace_thread_switched_in
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    pop {r0, r1}
    mov lr, r1
#else
    pop {r0, lr}
#endif
#endif /* CONFIG_TRACING */

    /*
     * Cortex-M: return from PendSV exception
     * Cortex-R: return to the caller (_IntExit or z_arm_svc)
     */
    bx lr

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) || \
  defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)

/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is used in the following occasions:
 * - IRQ offloading
 * - Kernel run-time exceptions
 * - System Calls (User mode)
 *
 * @return N/A
 */
SECTION_FUNC(TEXT, z_arm_svc)
  /* Use EXC_RETURN state to find out if stack frame is on the
   * MSP or PSP
   */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
  movs r0, #0x4
  mov r1, lr
  tst r1, r0
  beq _stack_frame_msp
  mrs r0, PSP
  bne _stack_frame_endif
_stack_frame_msp:
  mrs r0, MSP
_stack_frame_endif:
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    tst lr, #0x4    /* did we come from thread mode ? */
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
    beq _do_syscall

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
    bne _oops

#endif /* CONFIG_USERSPACE */

    cmp r1, #2
    beq _oops

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
    b z_arm_int_exit
#endif

_oops:
    push {r0, lr}
    bl z_do_kernel_oops
    /* return from SVC exception is done here */
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
_do_syscall:
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    movs r3, #24
    ldr r1, [r0, r3]   /* grab address of PC from stack frame */
    mov r8, r1
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    ldr r8, [r0, #24]   /* grab address of PC from stack frame */
#endif
    ldr r1, =z_arm_do_syscall
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    str r1, [r0, r3]   /* overwrite the PC to point to z_arm_do_syscall */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    str r1, [r0, #24]   /* overwrite the PC to point to z_arm_do_syscall */
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    ldr r3, =K_SYSCALL_LIMIT
    cmp r6, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    /* validate syscall limit */
    ldr ip, =K_SYSCALL_LIMIT
    cmp r6, ip
#endif
    blt valid_syscall_id

    /* bad syscall id.  Set arg1 to bad id and set call_id to SYSCALL_BAD */
    str r6, [r0]
    ldr r6, =K_SYSCALL_BAD

    /* Bad syscalls treated as valid syscalls with ID K_SYSCALL_BAD. */

valid_syscall_id:
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

    /* return from SVC to the modified LR - z_arm_do_syscall */
    bx lr
#endif /* CONFIG_USERSPACE */

#elif defined(CONFIG_ARMV7_R)

/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is used in the following occasions:
 * - Cooperative context switching
 * - IRQ offloading
 * - Kernel run-time exceptions
 *
 * @return N/A
 */
SECTION_FUNC(TEXT, z_arm_svc)
    /*
     * Switch to system mode to store r0-r3 to the process stack pointer.
     * Save r12 and the lr as we could be swapping in another process and
     * returning to a different location.
     */
    push {r4, r5}
    mov r4, r12
    mov r5, lr

    cps #MODE_SYS
    stmdb sp!, {r0-r5}
    cps #MODE_SVC

    pop {r4, r5}

    /* Get SVC number */
    mrs r0, spsr
    tst r0, #0x20

    ldreq r1, [lr, #-4]
    biceq r1, #0xff000000
    beq demux

    ldr r1, [lr, #-2]
    bic r1, #0xff00

   /*
    * grab service call number:
    * 0: context switch
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * Planned implementation of system calls for memory protection will
    * expand this case.
    */
demux:
    cmp r1, #_SVC_CALL_CONTEXT_SWITCH
    beq _context_switch

    cmp r1, #_SVC_CALL_RUNTIME_EXCEPT
    beq _oops

#if CONFIG_IRQ_OFFLOAD
    blx z_irq_do_offload  /* call C routine which executes the offload */

    /* exception return is done in z_arm_int_exit() */
    mov r0, #RET_FROM_SVC
    b z_arm_int_exit
#endif

_context_switch:
    /* handler mode exit, to PendSV */
    bl z_arm_pendsv

    mov r0, #RET_FROM_SVC
    b z_arm_int_exit

_oops:
    push {r0, lr}
    blx z_do_kernel_oops
    pop {r0, lr}
    cpsie i
    movs pc, lr

GTEXT(z_arm_cortex_r_svc)
SECTION_FUNC(TEXT, z_arm_cortex_r_svc)
    svc #0
    bx lr

#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
