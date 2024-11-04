/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2017-2019 Nordic Semiconductor ASA.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARM Cortex-A and Cortex-R
 *
 * This module implements the routines necessary for thread context switching
 * on ARM Cortex-A and Cortex-R CPUs.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include <zephyr/kernel.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_svc)
GTEXT(z_arm_do_swap)
GTEXT(z_do_kernel_oops)
#if defined(CONFIG_USERSPACE)
GTEXT(z_arm_do_syscall)
#endif

GDATA(_kernel)

/**
 *
 * @brief Routine to handle context switches
 *
 * For Cortex-R, this function is directly called either by z_arm_{exc,int}_exit
 * in case of preemption, or z_arm_svc in case of cooperative switching.
 */

SECTION_FUNC(TEXT, z_arm_do_swap)

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
    /* Register the context switch */
    push {r0, lr}
    bl z_thread_mark_switched_out
    pop {r0, lr}
#endif /* CONFIG_INSTRUMENT_THREAD_SWITCHING */

    /* load current _cpu into r1 and current k_thread into r2 */
    get_cpu r1
    ldr r2, [r1, #___cpu_t_current_OFFSET]

#if defined(CONFIG_ARM_STORE_EXC_RETURN)
    /* Store LSB of LR (EXC_RETURN) to the thread's 'mode' word. */
    strb lr, [r2, #_thread_offset_to_mode_exc_return]
#endif

    /* addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    /* Store rest of process context */
    cps #MODE_SYS
    stm r0, {r4-r11, sp}
    cps #MODE_SVC

#if defined(CONFIG_FPU_SHARING)
    ldrb r0, [r2, #_thread_offset_to_user_options]
    tst r0, #K_FP_REGS /* _current->base.user_options & K_FP_REGS */
    beq out_fp_inactive

    mov ip, #FPEXC_EN
    vmsr fpexc, ip

    /*
     * If the float context pointer is not null, then the VFP has not been
     * used since this thread has used it.  Consequently, the caller-saved
     * float registers have not been saved away, so write them to the
     * exception stack frame.
     */
    ldr r0, [r1, #___cpu_t_fp_ctx_OFFSET]
    cmp r0, #0
    beq out_store_thread_context

    vstmia r0!, {s0-s15}
#ifdef CONFIG_VFP_FEATURE_REGS_S64_D32
    vstmia r0!, {d16-d31}
#endif
    vmrs r3, fpscr
    stm r0, {r3, ip}

out_store_thread_context:
    /* Store s16-s31 to thread context */
    add r0, r2, #_thread_offset_to_preempt_float
    vstmia r0, {s16-s31}

    mov ip, #0
    vmsr fpexc, ip

out_fp_inactive:
    /*
     * The floating context has now been saved to the exception stack
     * frame, so zero out the global pointer to note this.
     */
    mov r0, #0
    str r0, [r1, #___cpu_t_fp_ctx_OFFSET]
#endif /* CONFIG_FPU_SHARING */

    /* fetch the thread to run from the ready queue cache */
    ldr r3, =_kernel
    ldr r2, [r3, #_kernel_offset_to_ready_q_cache]

    str r2, [r1, #___cpu_t_current_OFFSET]

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
    /* Grab the TLS pointer */
    ldr r4, =_thread_offset_to_tls
    adds r4, r2, r4
    ldr r0, [r4]

    /* Store TLS pointer in the "Process ID" register.
     * TPIDRURW is used as a base pointer to all
     * thread variables with offsets added by toolchain.
     */
    mcr p15, 0, r0, c13, c0, 2
#endif

#if defined(CONFIG_ARM_STORE_EXC_RETURN)
    /* Restore EXC_RETURN value. */
    ldrsb lr, [r2, #_thread_offset_to_mode_exc_return]
#endif

    /* Restore previous interrupt disable state (irq_lock key)
     * (We clear the arch.basepri field after restoring state)
     */
    ldr r0, [r2, #_thread_offset_to_basepri]
    movs r3, #0
    str r3, [r2, #_thread_offset_to_basepri]

    /* addr of callee-saved regs in thread in r0 */
    ldr r0, =_thread_offset_to_callee_saved
    add r0, r2

    /* restore r4-r11 and sp for incoming thread */
    cps #MODE_SYS
    ldm r0, {r4-r11, sp}
    cps #MODE_SVC

#if defined(CONFIG_FPU_SHARING)
    ldrb r0, [r2, #_thread_offset_to_user_options]
    tst r0, #K_FP_REGS /* _current->base.user_options & K_FP_REGS */
    beq in_fp_inactive

    mov r3, #FPEXC_EN
    vmsr fpexc, r3

    /* Restore s16-s31 from thread context */
    add r0, r2, #_thread_offset_to_preempt_float
    vldmia r0, {s16-s31}

    mov r3, #0
    vmsr fpexc, r3

in_fp_inactive:
#endif /* CONFIG_FPU_SHARING */

#if defined (CONFIG_ARM_MPU)
    /* r2 contains k_thread */
    mov r0, r2
    /* Re-program dynamic memory map */
    push {r0, lr}
    bl z_arm_configure_dynamic_mpu_regions
    pop {r0, lr}
#endif

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
    /* Register the context switch */
    push {r0, lr}
    bl z_thread_mark_switched_in
    pop {r0, lr}
#endif /* CONFIG_INSTRUMENT_THREAD_SWITCHING */

    /*
     * Cortex-R: return to the caller (z_arm_{exc,int}_exit, or z_arm_svc)
     */
    bx lr

#if defined(CONFIG_FPU_SHARING)
#define FPU_SF_SIZE ___fpu_t_SIZEOF
#else
#define FPU_SF_SIZE 0
#endif

/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is used in the following occasions:
 * - Cooperative context switching
 * - IRQ offloading
 * - Kernel run-time exceptions
 *
 */
SECTION_FUNC(TEXT, z_arm_svc)
#if defined(CONFIG_USERSPACE)
    /* Determine if incoming thread was in user context */
    push {r0}
    mrs r0, spsr
    and r0, #MODE_MASK
    cmp r0, #MODE_USR
    bne svc_system_thread

    get_cpu r0
    ldr r0, [r0, #___cpu_t_current_OFFSET]

    /* Save away user stack pointer */
    cps #MODE_SYS
    str sp, [r0, #_thread_offset_to_sp_usr] /* sp_usr */

    /* Switch to privileged stack */
    ldr sp, [r0, #_thread_offset_to_priv_stack_end] /* priv stack end */
    cps #MODE_SVC

svc_system_thread:
    pop {r0}
#endif

    /*
     * Switch to system mode to store r0-r3 to the process stack pointer.
     * Save r12 and the lr as we could be swapping in another process and
     * returning to a different location.
     */
    srsdb #MODE_SYS!
    cps #MODE_SYS
    push {r0-r3, r12, lr}

#if defined(CONFIG_FPU_SHARING)
    sub sp, sp, #___fpu_t_SIZEOF

    /*
     * Note that this handler was entered with the VFP unit enabled.
     * The undefined instruction handler uses this to know that it
     * needs to save the current floating context.
     */
    vmrs r0, fpexc
    str r0, [sp, #___fpu_t_SIZEOF - 4]
    tst r0, #FPEXC_EN
    beq _vfp_not_enabled
    vmrs r0, fpscr
    str r0, [sp, #___fpu_t_SIZEOF - 8]

    /* Disable VFP */
    mov r0, #0
    vmsr fpexc, r0

_vfp_not_enabled:
    /*
     * Mark where to store the floating context for the undefined
     * instruction handler
     */
    get_cpu r2
    ldr r0, [r2, #___cpu_t_fp_ctx_OFFSET]
    cmp r0, #0
    streq sp, [r2, #___cpu_t_fp_ctx_OFFSET]
#endif /* CONFIG_FPU_SHARING */

    mov ip, sp

    cps #MODE_SVC

    /*
     * Store lr_svc to the SVC mode stack. This value will be restored prior to
     * exiting the SVC call in z_arm_int_exit.
     */
    push {lr}

    /* Align stack at double-word boundary */
    /* TODO: Question, why push {r2, r3} here */
    and r3, sp, #4
    sub sp, sp, r3
    push {r2, r3}

    /* Increment interrupt nesting count */
    get_cpu r2
    ldr r0, [r2, #___cpu_t_nested_OFFSET]
    add r0, r0, #1
    str r0, [r2, #___cpu_t_nested_OFFSET]

    /* Get SVC number */
    mrs r0, spsr
    tst r0, #0x20

    ldreq r1, [lr, #-4]
    biceq r1, #0xff000000
    beq demux

    ldr r1, [lr, #-2]
    and r1, #0xff

   /*
    * grab service call number:
    * 0: context switch
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * 3: system calls for memory protection
    */
demux:

#if defined(CONFIG_USERSPACE)
    cmp r1, #_SVC_CALL_SYSTEM_CALL
    beq _do_syscall
#endif

    cmp r1, #_SVC_CALL_CONTEXT_SWITCH
    beq _context_switch

    cmp r1, #_SVC_CALL_RUNTIME_EXCEPT
    beq _oops

#if CONFIG_IRQ_OFFLOAD
    blx z_irq_do_offload  /* call C routine which executes the offload */

    /* exception return is done in z_arm_int_exit() */
    b z_arm_int_exit
#endif

_context_switch:
    /* handler mode exit, to PendSV */
    bl z_arm_do_swap

    b z_arm_int_exit

_oops:
    /*
     * Pass the exception frame to z_do_kernel_oops.  r0 contains the
     * exception reason.
     */
    cps #MODE_SYS
    mov r0, sp
    cps #MODE_SVC
    bl z_do_kernel_oops
    b z_arm_int_exit

#if defined(CONFIG_USERSPACE)
    /*
     * System call will setup a jump to the _do_arm_syscall function
     * running in system mode when returning from the exception.
     *
     * There is some trickery involved here because we have to preserve
     * the original PC value so that we can return back to the caller of
     * the SVC.
     *
     * On SVC exception, the USER/SYSTEM stack looks like the following:
     * { possible FPU space } - r0 - r1 - r2 - r3 - r12 - LR - PC - SPSR
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
    /* grab address of LR from stack frame */
    ldr r8, [ip, #(FPU_SF_SIZE + ___basic_sf_t_pc_OFFSET)]

    /* Make the exception return to system state */
    ldr r1, [ip, #(FPU_SF_SIZE + ___basic_sf_t_xpsr_OFFSET)]

    /* If leaving thumb mode, set the return address to thumb mode */
    tst r1, #T_BIT
    orrne r8, #1

    bic r1, #(MODE_MASK | T_BIT)
    orr r1, r1, #MODE_SYS
    str r1, [ip, #(FPU_SF_SIZE + ___basic_sf_t_xpsr_OFFSET)]

    /*
     * Store the address of z_arm_do_syscall for the exit so the exception
     * return goes there in system state.
     */
    ldr r1, =z_arm_do_syscall
    str r1, [ip, #(FPU_SF_SIZE + ___basic_sf_t_pc_OFFSET)]

    /* validate syscall limit, only set priv mode if valid */
    ldr ip, =K_SYSCALL_LIMIT
    cmp r6, ip
    blo valid_syscall_id

    /* bad syscall id.  Set arg0 to bad id and set call_id to SYSCALL_BAD */
    cps #MODE_SYS
    str r6, [sp]
    cps #MODE_SVC
    ldr r6, =K_SYSCALL_BAD

valid_syscall_id:
    get_cpu r0
    ldr r0, [r0, #___cpu_t_current_OFFSET]
    ldr r1, [r0, #_thread_offset_to_mode]
    bic r1, #1
    /* Store (privileged) mode in thread's mode state variable */
    str r1, [r0, #_thread_offset_to_mode]
    dsb

    /* ISB is not strictly necessary here (stack pointer is not being
     * touched), but it's recommended to avoid executing pre-fetched
     * instructions with the previous privilege.
     */
    isb

    /* Return to _arm_do_syscall in system state. */
    b z_arm_int_exit
#endif

GTEXT(z_arm_cortex_r_svc)
SECTION_FUNC(TEXT, z_arm_cortex_r_svc)
    svc #_SVC_CALL_CONTEXT_SWITCH
    bx lr
