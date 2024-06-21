/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A and Cortex-R wrapper for ISRs with parameter
 *
 * Wrapper installed in vector table for handling dynamic interrupts that accept
 * a parameter.
 */
/*
 * Tell armclang that stack alignment are ensured.
 */
.eabi_attribute Tag_ABI_align_preserved, 1

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sw_isr_table.h>
#include "macro_priv.inc"


_ASM_FILE_PROLOGUE

GDATA(_sw_isr_table)

GTEXT(_isr_wrapper)
GTEXT(z_arm_int_exit)

#ifndef CONFIG_USE_SWITCH
/**
 *
 * @brief Wrapper around ISRs when inserted in software ISR table
 *
 * When inserted in the vector table, _isr_wrapper() demuxes the ISR table
 * using the running interrupt number as the index, and invokes the registered
 * ISR with its corresponding argument. When returning from the ISR, it
 * determines if a context switch needs to happen (see documentation for
 * z_arm_pendsv()) and pends the PendSV exception if so: the latter will
 * perform the context switch itself.
 *
 */
SECTION_FUNC(TEXT, _isr_wrapper)

#if defined(CONFIG_USERSPACE)
	/* See comment below about svc stack usage */
	cps #MODE_SVC
	push {r0}

	/* Determine if interrupted thread was in user context */
	cps #MODE_IRQ
	mrs r0, spsr
	and r0, #MODE_MASK
	cmp r0, #MODE_USR
	bne isr_system_thread

	get_cpu r0
	ldr r0, [r0, #___cpu_t_current_OFFSET]

	/* Save away user stack pointer */
	cps #MODE_SYS
	str sp, [r0, #_thread_offset_to_sp_usr] /* sp_usr */

	/* Switch to privileged stack */
	ldr sp, [r0, #_thread_offset_to_priv_stack_end] /* priv stack end */

isr_system_thread:
	cps #MODE_SVC
	pop {r0}
	cps #MODE_IRQ
#endif

	/*
	 * Save away r0-r3, r12 and lr_irq for the previous context to the
	 * process stack since they are clobbered here.  Also, save away lr
	 * and spsr_irq since we may swap processes and return to a different
	 * thread.
	 */
	sub lr, lr, #4
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

	/*
	 * Use SVC mode stack for predictable interrupt behaviour; running ISRs
	 * in the SYS/USR mode stack (i.e. interrupted thread stack) leaves the
	 * ISR stack usage at the mercy of the interrupted thread and this can
	 * be prone to stack overflows if any of the ISRs and/or preemptible
	 * threads have high stack usage.
	 *
	 * When userspace is enabled, this also prevents leaking privileged
	 * information to the user mode.
	 */
	cps #MODE_SVC

	/*
	 * Preserve lr_svc which may contain the branch return address of the
	 * interrupted context in case of a nested interrupt. This value will
	 * be restored prior to exiting the interrupt in z_arm_int_exit.
	 */
	push {lr}

	/* Align stack at double-word boundary */
	and r3, sp, #4
	sub sp, sp, r3
	push {r2, r3}

	/* Increment interrupt nesting count */
	get_cpu r2
	ldr r0, [r2, #___cpu_t_nested_OFFSET]
	add r0, r0, #1
	str r0, [r2, #___cpu_t_nested_OFFSET]

#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_enter
#endif

#ifdef CONFIG_PM
	/*
	 * All interrupts are disabled when handling idle wakeup.  For tickless
	 * idle, this ensures that the calculation and programming of the
	 * device for the next timer deadline is not interrupted.  For
	 * non-tickless idle, this ensures that the clearing of the kernel idle
	 * state is not interrupted.  In each case, pm_system_resume
	 * is called with interrupts disabled.
	 */

	/* is this a wakeup from idle ? */
	ldr r2, =_kernel
	/* requested idle duration, in ticks */
	ldr r0, [r2, #_kernel_offset_to_idle]
	cmp r0, #0

	beq _idle_state_cleared
	movs r1, #0
	/* clear kernel idle state */
	str r1, [r2, #_kernel_offset_to_idle]
	bl pm_system_resume
_idle_state_cleared:

#endif /* CONFIG_PM */

	/* Get active IRQ number from the interrupt controller */
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl arm_gic_get_active
#else
	bl z_soc_irq_get_active
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */
	push {r0, r1}
	lsl r0, r0, #3	/* table is 8-byte wide */

	/*
	 * Enable interrupts to allow nesting.
	 *
	 * Note that interrupts are disabled up to this point on the ARM
	 * architecture variants other than the Cortex-M. It is also important
	 * to note that most interrupt controllers require that the nested
	 * interrupts are handled after the active interrupt is acknowledged;
	 * this is be done through the `get_active` interrupt controller
	 * interface function.
	 */
	cpsie i

	/*
	 * Skip calling the isr if it is a spurious interrupt.
	 */
	mov r1, #CONFIG_NUM_IRQS
	lsl r1, r1, #3
	cmp r0, r1
	bge spurious_continue

	ldr r1, =_sw_isr_table
	add r1, r1, r0	/* table entry: ISRs must have their MSB set to stay
			 * in thumb mode */

	ldm r1!,{r0,r3}	/* arg in r0, ISR in r3 */
	blx r3		/* call ISR */

spurious_continue:
	/* Signal end-of-interrupt */
	pop {r0, r1}
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl arm_gic_eoi
#else
	bl z_soc_irq_eoi
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_exit
#endif

	/* Use 'bx' instead of 'b' because 'bx' can jump further, and use
	 * 'bx' instead of 'blx' because exception return is done in
	 * z_arm_int_exit() */
	ldr r1, =z_arm_int_exit
	bx r1

#else
/**
 *
 * @brief Wrapper around ISRs when inserted in software ISR table
 *
 * When inserted in the vector table, _isr_wrapper() demuxes the ISR table
 * using the running interrupt number as the index, and invokes the registered
 * ISR with its corresponding argument. When returning from the ISR, it
 * determines if a context switch needs to happen and invoke the arch_switch
 * function if so.
 *
 */
SECTION_FUNC(TEXT, _isr_wrapper)
	sub lr, #4
	z_arm_cortex_ar_enter_exc

	/* Increment interrupt nesting count */
	get_cpu r2
	ldr r0, [r2, #___cpu_t_nested_OFFSET]
	add r0, #1
	str r0, [r2, #___cpu_t_nested_OFFSET]

	/* If not nested: switch to IRQ stack and save current sp on it. */
	cmp r0, #1
	bhi 1f
	mov r0, sp
	cps #MODE_IRQ
	push {r0}
1:
#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_enter
#endif /* CONFIG_TRACING_ISR */

#ifdef CONFIG_PM
	/*
	 * All interrupts are disabled when handling idle wakeup.  For tickless
	 * idle, this ensures that the calculation and programming of the
	 * device for the next timer deadline is not interrupted.  For
	 * non-tickless idle, this ensures that the clearing of the kernel idle
	 * state is not interrupted.  In each case, pm_system_resume
	 * is called with interrupts disabled.
	 */

	/* is this a wakeup from idle ? */
	ldr r2, =_kernel
	/* requested idle duration, in ticks */
	ldr r0, [r2, #_kernel_offset_to_idle]
	cmp r0, #0

	beq _idle_state_cleared
	movs r1, #0
	/* clear kernel idle state */
	str r1, [r2, #_kernel_offset_to_idle]
	bl pm_system_resume
_idle_state_cleared:
#endif /* CONFIG_PM */

	/* Get active IRQ number from the interrupt controller */
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl arm_gic_get_active
#else
	bl z_soc_irq_get_active
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

	push {r0, r1}
	lsl r0, r0, #3	/* table is 8-byte wide */

	/*
	 * Skip calling the isr if it is a spurious interrupt.
	 */
	mov r1, #CONFIG_NUM_IRQS
	lsl r1, r1, #3
	cmp r0, r1
	bge spurious_continue

	ldr r1, =_sw_isr_table
	add r1, r1, r0	/* table entry: ISRs must have their MSB set to stay
			 * in thumb mode */
	ldm r1!,{r0,r3}	/* arg in r0, ISR in r3 */

	/*
	 * Enable and disable interrupts again to allow nested in exception handlers.
	 */
	cpsie i
	blx r3		/* call ISR */
	cpsid i

spurious_continue:
	/* Signal end-of-interrupt */
	pop {r0, r1}
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl arm_gic_eoi
#else
	bl z_soc_irq_eoi
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */

#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_exit
#endif

GTEXT(z_arm_cortex_ar_irq_done)
z_arm_cortex_ar_irq_done:
	/* Decrement interrupt nesting count */
	get_cpu r2
	ldr r0, [r2, #___cpu_t_nested_OFFSET]
	sub r0, r0, #1
	str r0, [r2, #___cpu_t_nested_OFFSET]
	/* Do not context switch if exiting a nested interrupt */
	cmp r0, #0
	bhi __EXIT_INT

	/* retrieve pointer to the current thread */
	pop {r0}
	cps #MODE_SYS
	mov sp, r0

	ldr r1, [r2, #___cpu_t_current_OFFSET]
	push {r1}
	mov r0, #0
	bl z_get_next_switch_handle

	pop {r1}
	cmp r0, #0
	beq __EXIT_INT

	/*
	 * Switch thread
	 * r0: new thread
	 * r1: old thread
	 */
	bl z_arm_context_switch

__EXIT_INT:

#ifdef CONFIG_STACK_SENTINEL
	bl z_check_stack_sentinel
#endif /* CONFIG_STACK_SENTINEL */

	b z_arm_cortex_ar_exit_exc

#endif
