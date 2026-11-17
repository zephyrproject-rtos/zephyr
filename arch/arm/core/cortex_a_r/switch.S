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

	add r2, r1, #_thread_offset_to_callee_saved

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

#ifdef CONFIG_SMP
	/* Make the saved state visible to other CPUs before the switch
	 * handle that z_sched_switch_spin() waits for.
	 */
	dmb
#endif
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

	add r2, r0, #_thread_offset_to_callee_saved
	ldm r2, {r4-r11, sp, lr}

	/* The last call is a tail call: no caller uses r0 on return */
#if defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
#if defined(CONFIG_ARM_MPU) && (defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD))
	/* Re-program dynamic memory map */
	push {r0, lr}
	bl z_arm_configure_dynamic_mpu_regions
	pop {r0, lr}
#endif
	b z_thread_mark_switched_in
#elif defined(CONFIG_ARM_MPU) && (defined(CONFIG_USERSPACE) || defined(CONFIG_MPU_STACK_GUARD))
	/* Re-program dynamic memory map */
	b z_arm_configure_dynamic_mpu_regions
#else
	bx lr
#endif

/**
 *
 * @brief Service call handler
 *
 * The service call (svc) is used in the following occasions:
 * - IRQ offloading
 * - Kernel run-time exceptions
 * - System calls
 *
 */
SECTION_FUNC(TEXT, z_arm_svc)
    z_arm_cortex_ar_enter_exc
#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
    ldr r0, [sp]
#endif

    /*
     * Get the SVC number from the instruction before the return address.
     * r0: exception stack frame, r2: CPU pointer, r3: return address,
     * ip: SPSR
     */
    ldr ip, [r0, #___basic_sf_t_xpsr_OFFSET]
    ldr r3, [r0, #___basic_sf_t_pc_OFFSET]
    tst ip, #T_BIT
    ldreq r1, [r3, #-4]
    biceq r1, #0xff000000
    beq demux

    ldr r1, [r3, #-2]
    and r1, #0xff

   /*
    * grab service call number:
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * 3: system calls for memory protection
    */
demux:
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
    z_arm_cortex_ar_save_extra
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
     * The exception stack frame, at r0, is:
     * r0 - r1 - r2 - r3 - r12 - LR - PC - SPSR
     *
     * Registers look like:
     * r0-r3 - args, in the frame
     * r4-r5 - args
     * r6 - call_id
     * r8 - set here to the return address of the SVC
     */
_do_syscall:
    mov r8, r3

    /* If leaving thumb mode, set the return address to thumb mode */
    tst ip, #T_BIT
    orrne r8, #1

    /* Make the exception return to system state */
    bic ip, #(MODE_MASK | T_BIT)
    orr ip, ip, #MODE_SYS
    str ip, [r0, #___basic_sf_t_xpsr_OFFSET]

    ldr r1, =z_arm_do_syscall
    str r1, [r0, #___basic_sf_t_pc_OFFSET]

    /* On a bad syscall id, set arg0 to that id and call_id to K_SYSCALL_BAD */
    ldr ip, =K_SYSCALL_LIMIT
    cmp r6, ip
    strhs r6, [r0]
    ldrhs r6, =K_SYSCALL_BAD

    b z_arm_cortex_ar_exit_exc_cpu
#endif

inv:
    z_arm_cortex_ar_save_extra
    mov r1, r0
    mov r0, #0 /* K_ERR_CPU_EXCEPTION */
    bl z_arm_fatal_error

    /* Return here only in case of recoverable error */
    b z_arm_cortex_ar_exit_exc
