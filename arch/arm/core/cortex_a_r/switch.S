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
	mcr 15, 0, r3, c13, c0, 2
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
	push {lr}
	bl z_thread_mark_switched_in
	pop {lr}
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
    * TODO 0: context switch
    * 1: irq_offload (if configured)
    * 2: kernel panic or oops (software generated fatal exception)
    * TODO 3: system calls for memory protection
    */
demux:
    cps #MODE_SYS

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

    /* If not nested: switch to IRQ stack and save current sp on it. */
    cmp r3, #1
    bhi 1f
    mov r0, sp
    cps #MODE_IRQ
    push {r0}

1:
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

inv:
    mov r0, #0 /* K_ERR_CPU_EXCEPTION */
    mov r1, sp
    bl z_arm_fatal_error

    /* Return here only in case of recoverable error */
    b z_arm_cortex_ar_exit_exc
