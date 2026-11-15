/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARM Cortex-A and Cortex-R (AArch32)
 *
 * This module implements the routines necessary for thread context switching
 * on ARM Cortex-A and Cortex-R CPUs.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>
#include <zephyr/syscall.h>
#include <zephyr/kernel.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_svc)
GTEXT(z_arm_context_switch)
GTEXT(z_do_kernel_oops)
GTEXT(z_arm_do_syscall)

/*
 * Routine to handle context switches
 *
 * This function is directly called either by _isr_wrapper() in case of
 * preemption, or arch_switch() in case of cooperative switching.
 *
 * void z_arm_context_switch(struct k_thread *new, struct k_thread *old);
 */
SECTION_FUNC(TEXT, z_arm_context_switch)

	ldr r2, =_thread_offset_to_callee_saved
	add r2, r1, r2

	stm r2, {r4-r11, sp, lr}

	/* save current thread's exception depth */
	get_cpu r2
	ldrb r3, [r2, #_cpu_offset_to_exc_depth]
	strb r3, [r1, #_thread_offset_to_exception_depth]

	/* retrieve next thread's exception depth */
	ldrb r3, [r0, #_thread_offset_to_exception_depth]
	strb r3, [r2, #_cpu_offset_to_exc_depth]

#if defined(CONFIG_FPU_SHARING)
	/*
	 * Do this after the exception depth of the next thread is set, and
	 * before the switch handle of the old one is stored: on SMP, this
	 * saves the old thread's FP context, which another CPU may load as
	 * soon as it sees that switch handle. lr is already saved.
	 */
	push {r0, r1}
	mov r0, r1
	bl z_arm_fpu_thread_context_switch
	pop {r0, r1}
#endif

	/* save old thread into switch handle which is required by
	 * z_sched_switch_spin().
	 *
	 * Note that this step must be done after all relevant state is
	 * saved.
	 */
	dsb
	str r1, [r1, #___thread_t_switch_handle_OFFSET]

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
	/* Grab the TLS pointer */
	ldr r3, [r0, #_thread_offset_to_tls]

	/* Store TLS pointer in the "Process ID" register.
	 * This register is used as a base pointer to all
	 * thread variables with offsets added by toolchain.
	 */
	mcr p15, 0, r3, c13, c0, 2
#endif

	ldr r2, =_thread_offset_to_callee_saved
	add r2, r0, r2
	ldm r2, {r4-r11, sp, lr}

#if defined (CONFIG_ARM_MPU)
	/* Re-program dynamic memory map */
	push {r0, lr}
	bl z_arm_configure_dynamic_mpu_regions
	pop {r0, lr}
#endif

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	push {r0, lr}
	bl z_thread_mark_switched_in
	pop {r0, lr}
#endif

	bx lr

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
    z_arm_cortex_ar_enter_exc

   /* Get SVC number */
    cps #MODE_SVC
    mrs r0, spsr
    tst r0, #0x20
    ldreq r1, [lr, #-4]
    biceq r1, #0xff000000
    beq demux

    ldr r1, [lr, #-2]
    and r1, #0xff

   /*
    * grab service call number:
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * 3: system calls for memory protection
    */
demux:
    cps #MODE_SYS

#if defined(CONFIG_USERSPACE)
    cmp r1, #_SVC_CALL_SYSTEM_CALL
    beq _do_syscall
#endif

    cmp r1, #_SVC_CALL_RUNTIME_EXCEPT
    beq _oops

#ifdef CONFIG_IRQ_OFFLOAD
    cmp r1, #_SVC_CALL_IRQ_OFFLOAD
    beq offload
    b inv
offload:
    get_cpu r2
    ldr r3, [r2, #___cpu_t_nested_OFFSET]
    add r3, r3, #1
    str r3, [r2, #___cpu_t_nested_OFFSET]

    irq_stack_enter r3
    blx z_irq_do_offload
    b z_arm_cortex_ar_irq_done
#endif
    b inv

_oops:
    /*
     * Pass the exception frame to z_do_kernel_oops.
     */
    mov r0, sp
    /* Zero callee_regs and exc_return (only used on Cortex-M) */
    mov r1, #0
    mov r2, #0
    bl z_do_kernel_oops
    /* An oops normally does not return, but fatal error handling may
     * return in test mode. Exit the exception in that case.
     */
    b z_arm_cortex_ar_exit_exc

#if defined(CONFIG_USERSPACE)
    /*
     * The exception return goes to z_arm_do_syscall in system mode,
     * which calls the handler and returns to the caller of the SVC.
     * The exception stack frame (in SYS mode, at sp) is:
     * r0 - r1 - r2 - r3 - r12 - LR - PC - SPSR
     *
     * Registers look like:
     * r0-r5 - args
     * r6 - call_id
     * r8 - set here to the return address of the SVC
     */
_do_syscall:
    ldr r8, [sp, #___basic_sf_t_pc_OFFSET]

    /* Make the exception return to system state */
    ldr r1, [sp, #___basic_sf_t_xpsr_OFFSET]

    /* If leaving thumb mode, set the return address to thumb mode */
    tst r1, #T_BIT
    orrne r8, #1

    bic r1, #(MODE_MASK | T_BIT)
    orr r1, r1, #MODE_SYS
    str r1, [sp, #___basic_sf_t_xpsr_OFFSET]

    ldr r1, =z_arm_do_syscall
    str r1, [sp, #___basic_sf_t_pc_OFFSET]

    /* validate syscall limit, only set priv mode if valid */
    ldr ip, =K_SYSCALL_LIMIT
    cmp r6, ip
    blo valid_syscall_id

    /* bad syscall id.  Set arg0 to bad id and set call_id to SYSCALL_BAD */
    str r6, [sp]
    ldr r6, =K_SYSCALL_BAD

valid_syscall_id:
    get_cpu r0
    ldr r0, [r0, #___cpu_t_current_OFFSET]
    ldr r1, [r0, #_thread_offset_to_mode]
    bic r1, #1
    /* Store (privileged) mode in thread's mode state variable */
    str r1, [r0, #_thread_offset_to_mode]
    dsb
    isb

    b z_arm_cortex_ar_exit_exc
#endif

inv:
    mov r0, #0 /* K_ERR_CPU_EXCEPTION */
    mov r1, sp
    bl z_arm_fatal_error

    /* Return here only in case of recoverable error */
    b z_arm_cortex_ar_exit_exc
