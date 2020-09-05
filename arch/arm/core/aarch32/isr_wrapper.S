/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M and Cortex-R wrapper for ISRs with parameter
 *
 * Wrapper installed in vector table for handling dynamic interrupts that accept
 * a parameter.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <sw_isr_table.h>


_ASM_FILE_PROLOGUE

GDATA(_sw_isr_table)

GTEXT(_isr_wrapper)
GTEXT(z_arm_int_exit)

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
 * @return N/A
 */
SECTION_FUNC(TEXT, _isr_wrapper)

#if defined(CONFIG_CPU_CORTEX_M)
	push {r0,lr}		/* r0, lr are now the first items on the stack */
#elif defined(CONFIG_CPU_CORTEX_R)
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

	/* Align stack at double-word boundary */
	and r3, sp, #4
	sub sp, sp, r3
	push {r2, r3}

	/* Increment interrupt nesting count */
	ldr r2, =_kernel
	ldr r0, [r2, #_kernel_offset_to_nested]
	add r0, r0, #1
	str r0, [r2, #_kernel_offset_to_nested]
#endif /* CONFIG_CPU_CORTEX_M */

#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_enter
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	/*
	 * All interrupts are disabled when handling idle wakeup.  For tickless
	 * idle, this ensures that the calculation and programming of the
	 * device for the next timer deadline is not interrupted.  For
	 * non-tickless idle, this ensures that the clearing of the kernel idle
	 * state is not interrupted.  In each case, z_sys_power_save_idle_exit
	 * is called with interrupts disabled.
	 */

#if defined(CONFIG_CPU_CORTEX_M)
	/*
	 * Disable interrupts to prevent nesting while exiting idle state. This
	 * is only necessary for the Cortex-M because it is the only ARM
	 * architecture variant that automatically enables interrupts when
	 * entering an ISR.
	 */
	cpsid i  /* PRIMASK = 1 */
#endif

	/* is this a wakeup from idle ? */
	ldr r2, =_kernel
	/* requested idle duration, in ticks */
	ldr r0, [r2, #_kernel_offset_to_idle]
	cmp r0, #0

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	beq _idle_state_cleared
	movs.n r1, #0
	/* clear kernel idle state */
	str r1, [r2, #_kernel_offset_to_idle]
	bl z_sys_power_save_idle_exit
_idle_state_cleared:

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	ittt ne
	movne	r1, #0
		/* clear kernel idle state */
		strne	r1, [r2, #_kernel_offset_to_idle]
		blne	z_sys_power_save_idle_exit
#elif defined(CONFIG_ARMV7_R)
	beq _idle_state_cleared
	movs r1, #0
	/* clear kernel idle state */
	str r1, [r2, #_kernel_offset_to_idle]
	bl z_sys_power_save_idle_exit
_idle_state_cleared:
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

#if defined(CONFIG_CPU_CORTEX_M)
	cpsie i		/* re-enable interrupts (PRIMASK = 0) */
#endif

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

#if defined(CONFIG_CPU_CORTEX_M)
	mrs r0, IPSR	/* get exception number */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	ldr r1, =16
	subs r0, r1	/* get IRQ number */
	lsls r0, #3	/* table is 8-byte wide */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	sub r0, r0, #16	/* get IRQ number */
	lsl r0, r0, #3	/* table is 8-byte wide */
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#elif defined(CONFIG_CPU_CORTEX_R)
	/* Get active IRQ number from the interrupt controller */
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl arm_gic_get_active
#else
	bl z_soc_irq_get_active
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */
	push {r0, r1}
	lsl r0, r0, #3	/* table is 8-byte wide */
#else
#error Unknown ARM architecture
#endif /* CONFIG_CPU_CORTEX_M */

#if !defined(CONFIG_CPU_CORTEX_M)
	/*
	 * Enable interrupts to allow nesting.
	 *
	 * Note that interrupts are disabled up to this point on the ARM
	 * architecture variants other than the Cortex-M. It is also important
	 * to note that that most interrupt controllers require that the nested
	 * interrupts are handled after the active interrupt is acknowledged;
	 * this is be done through the `get_active` interrupt controller
	 * interface function.
	 */
	cpsie i
#endif /* !CONFIG_CPU_CORTEX_M */

	ldr r1, =_sw_isr_table
	add r1, r1, r0	/* table entry: ISRs must have their MSB set to stay
			 * in thumb mode */

	ldm r1!,{r0,r3}	/* arg in r0, ISR in r3 */
	blx r3		/* call ISR */

#if defined(CONFIG_CPU_CORTEX_R)
	/* Signal end-of-interrupt */
	pop {r0, r1}
#if !defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
	bl arm_gic_eoi
#else
	bl z_soc_irq_eoi
#endif /* !CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER */
#endif /* CONFIG_CPU_CORTEX_R */

#ifdef CONFIG_TRACING_ISR
	bl sys_trace_isr_exit
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop {r0, r3}
	mov lr, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	pop {r0, lr}
#elif defined(CONFIG_ARMV7_R)
	/*
	 * r0 and lr_irq were saved on the process stack since a swap could
	 * happen.  exc_exit will handle getting those values back
	 * from the process stack to return to the correct location
	 * so there is no need to do anything here.
	 */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	/* Use 'bx' instead of 'b' because 'bx' can jump further, and use
	 * 'bx' instead of 'blx' because exception return is done in
	 * z_arm_int_exit() */
	ldr r1, =z_arm_int_exit
	bx r1
