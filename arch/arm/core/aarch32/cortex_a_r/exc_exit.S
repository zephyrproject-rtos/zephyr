/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A and Cortex-R exception/interrupt exit API
 *
 * Provides functions for performing kernel handling when exiting exceptions,
 * or interrupts that are installed directly in the vector table (i.e. that are
 * not wrapped around by _isr_wrapper()).
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_exc_exit)
GTEXT(z_arm_int_exit)
GTEXT(z_arm_pendsv)
GDATA(_kernel)

.macro userspace_exc_exit
#if defined(CONFIG_USERSPACE)
	cps #MODE_SVC
	sub sp, #8
	push {r0-r1}

	/*
	 * Copy return state from sys/usr state onto the svc stack.
	 * We have to put $sp_usr back into $sp since we switched to
	 * the privileged stack on exception entry.  The return state
	 * is on the privileged stack so it needs to be copied to the
	 * svc stack since we cannot trust the usr stack.
	 */
	cps #MODE_SYS
	pop {r0-r1}

	cps #MODE_SVC
	str r0, [sp, #8]
	str r1, [sp, #12]

	/* Only switch the stacks if returning to a user thread */
	and r1, #MODE_MASK
	cmp r1, #MODE_USR
	bne system_thread_exit\@

	/* Restore user stack pointer */
	ldr r0, =_kernel
	ldr r0, [r0, #_kernel_offset_to_current]
	cps #MODE_SYS
	ldr sp, [r0, #_thread_offset_to_sp_usr] /* sp_usr */
	cps #MODE_SVC
system_thread_exit\@:
	pop {r0-r1}
#endif
.endm

.macro fpu_exc_exit
#if defined(CONFIG_FPU_SHARING)
	/*
	 * If the floating point context pointer is null, then a context was
	 * saved so restore the float context from the exception stack frame.
	 */
	ldr r2, =_kernel
	ldr r1, [r2, #_kernel_offset_to_fp_ctx]
	cmp r1, #0
	beq vfp_restore\@

	/*
	 * If leaving the last interrupt context, remove the floating point
	 * context pointer.
	 */
	cmp r0, #0
	moveq r1, #0
	streq r1, [r2, #_kernel_offset_to_fp_ctx]
	b vfp_exit\@

vfp_restore\@:
	add r3, sp, #___fpu_sf_t_fpscr_OFFSET
	ldm r3, {r1, r2}
	tst r2, #FPEXC_EN
	beq vfp_exit\@

	vmsr fpexc, r2
	vmsr fpscr, r1
	vldmia sp, {s0-s15}

vfp_exit\@:
	/* Leave the VFP disabled when leaving */
	mov r1, #0
	vmsr fpexc, r1

	add sp, sp, #___fpu_t_SIZEOF
#endif
.endm

/**
 * @brief Kernel housekeeping when exiting interrupt handler installed directly
 *        in the vector table
 *
 * Kernel allows installing interrupt handlers (ISRs) directly into the vector
 * table to get the lowest interrupt latency possible. This allows the ISR to
 * be invoked directly without going through a software interrupt table.
 * However, upon exiting the ISR, some kernel work must still be performed,
 * namely possible context switching. While ISRs connected in the software
 * interrupt table do this automatically via a wrapper, ISRs connected directly
 * in the vector table must invoke z_arm_int_exit() as the *very last* action
 * before returning.
 *
 * e.g.
 *
 * void myISR(void)
 * {
 * 	printk("in %s\n", __FUNCTION__);
 * 	doStuff();
 * 	z_arm_int_exit();
 * }
 */
SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, z_arm_int_exit)

#ifdef CONFIG_STACK_SENTINEL
	bl z_check_stack_sentinel
#endif /* CONFIG_STACK_SENTINEL */

#ifdef CONFIG_PREEMPT_ENABLED
	/* Do not context switch if exiting a nested interrupt */
	ldr r3, =_kernel
	ldr r0, [r3, #_kernel_offset_to_nested]
	cmp r0, #1
	bhi __EXIT_INT

	ldr r1, [r3, #_kernel_offset_to_current]
	ldr r0, [r3, #_kernel_offset_to_ready_q_cache]
	cmp r0, r1
	blne z_arm_pendsv
__EXIT_INT:
#endif /* CONFIG_PREEMPT_ENABLED */

	/* Disable nested interrupts while exiting */
	cpsid i

	/* Decrement interrupt nesting count */
	ldr r2, =_kernel
	ldr r0, [r2, #_kernel_offset_to_nested]
	sub r0, r0, #1
	str r0, [r2, #_kernel_offset_to_nested]

	/* Restore previous stack pointer */
	pop {r2, r3}
	add sp, sp, r3

	/*
	 * Restore lr_svc stored into the SVC mode stack by the mode entry
	 * function. This ensures that the return address of the interrupted
	 * context is preserved in case of interrupt nesting.
	 */
	pop {lr}

	/*
	 * Restore r0-r3, r12 and lr_irq stored into the process stack by the
	 * mode entry function. These registers are saved by _isr_wrapper for
	 * IRQ mode and z_arm_svc for SVC mode.
	 *
	 * r0-r3 are either the values from the thread before it was switched
	 * out or they are the args to _new_thread for a new thread.
	 */
	cps #MODE_SYS

#if defined(CONFIG_FPU_SHARING)
	fpu_exc_exit
#endif

	pop {r0-r3, r12, lr}
	userspace_exc_exit
	rfeia sp!

/**
 * @brief Kernel housekeeping when exiting exception handler
 *
 * The exception exit routine performs appropriate housekeeping tasks depending
 * on the mode of exit:
 *
 * If exiting a nested or non-fatal exception, the exit routine restores the
 * saved exception stack frame to resume the excepted context.
 *
 * If exiting a non-nested fatal exception, the exit routine, assuming that the
 * current faulting thread is aborted, discards the saved exception stack
 * frame containing the aborted thread context and switches to the next
 * scheduled thread.
 *
 * void z_arm_exc_exit(bool fatal)
 *
 * @param fatal True if exiting from a fatal fault; otherwise, false
 */
SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, z_arm_exc_exit)
	/* Do not context switch if exiting a nested exception */
	ldr r3, =_kernel
	ldr r1, [r3, #_kernel_offset_to_nested]
	cmp r1, #1
	bhi __EXIT_EXC

	/* If the fault is not fatal, return to the current thread context */
	cmp r0, #0
	beq __EXIT_EXC

	/*
	 * If the fault is fatal, the current thread must have been aborted by
	 * the exception handler. Clean up the exception stack frame and switch
	 * to the next scheduled thread.
	 */

	/* Clean up exception stack frame */
#if defined(CONFIG_FPU_SHARING)
	add sp, sp, #___fpu_t_SIZEOF
#endif
	add sp, #32

	/*
	 * Switch in the next scheduled thread.
	 *
	 * Note that z_arm_pendsv must be called in the SVC mode because it
	 * switches to the SVC mode during context switch and returns to the
	 * caller using lr_svc.
	 */
	cps #MODE_SVC
	bl z_arm_pendsv

	/* Decrement exception nesting count */
	ldr r3, =_kernel
	ldr r0, [r3, #_kernel_offset_to_nested]
	sub r0, r0, #1
	str r0, [r3, #_kernel_offset_to_nested]

	/* Return to the switched thread */
	cps #MODE_SYS
#if defined(CONFIG_FPU_SHARING)
	fpu_exc_exit
#endif
	pop {r0-r3, r12, lr}
	userspace_exc_exit
	rfeia sp!

__EXIT_EXC:
	/* Decrement exception nesting count */
	ldr r0, [r3, #_kernel_offset_to_nested]
	sub r0, r0, #1
	str r0, [r3, #_kernel_offset_to_nested]

#if defined(CONFIG_FPU_SHARING)
	add sp, sp, #___fpu_t_SIZEOF
#endif
	/*
	 * Restore r0-r3, r12, lr, lr_und and spsr_und from the exception stack
	 * and return to the current thread.
	 */
	ldmia sp, {r0-r3, r12, lr}^
	add sp, #24
	rfeia sp!
