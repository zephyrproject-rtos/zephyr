/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
	 * Save away r0-r3 from previous context to the process stack since
	 * they are clobbered here.  Also, save away lr since we may swap
	 * processes and return to a different thread.
	 */
	push {r4, r5}
	mov r4, r12
	sub r5, lr, #4

	cps #MODE_SYS
	stmdb sp!, {r0-r5}
	cps #MODE_IRQ

	pop {r4, r5}
#endif

#ifdef CONFIG_EXECUTION_BENCHMARKING
	bl read_timer_start_of_isr
#endif

#ifdef CONFIG_TRACING
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
	cpsid i  /* PRIMASK = 1 */

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

	cpsie i		/* re-enable interrupts (PRIMASK = 0) */
#endif

#if defined(CONFIG_CPU_CORTEX_M)
	mrs r0, IPSR	/* get exception number */
#endif
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	ldr r1, =16
	subs r0, r1	/* get IRQ number */
	lsls r0, #3	/* table is 8-byte wide */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	sub r0, r0, #16	/* get IRQ number */
	lsl r0, r0, #3	/* table is 8-byte wide */
#elif defined(CONFIG_ARMV7_R)
	/*
	 * Cortex-R only has one IRQ line so the main handler will be at
	 * offset 0 of the table.
	 */
	mov r0, #0
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	ldr r1, =_sw_isr_table
	add r1, r1, r0	/* table entry: ISRs must have their MSB set to stay
			 * in thumb mode */

	ldm r1!,{r0,r3}	/* arg in r0, ISR in r3 */
#ifdef CONFIG_EXECUTION_BENCHMARKING
	stm sp!,{r0-r3} /* Save r0 to r3 into stack */
	push {r0, lr}
	bl read_timer_end_of_isr
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop {r0, r3}
	mov lr,r3
#else
	pop {r0, lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	ldm sp!,{r0-r3} /* Restore r0 to r3 regs */
#endif /* CONFIG_EXECUTION_BENCHMARKING */
	blx r3		/* call ISR */

#ifdef CONFIG_TRACING
	bl sys_trace_isr_exit
#endif

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop {r0, r3}
	mov lr, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	pop {r0, lr}
#elif defined(CONFIG_ARMV7_R)
	/*
	 * r0,lr were saved on the process stack since a swap could
	 * happen.  exc_exit will handle getting those values back
	 * from the process stack to return to the correct location
	 * so there is no need to do anything here.
	 */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

#if defined(CONFIG_CPU_CORTEX_R)
	mov r0, #RET_FROM_IRQ
#endif

	/* Use 'bx' instead of 'b' because 'bx' can jump further, and use
	 * 'bx' instead of 'blx' because exception return is done in
	 * z_arm_int_exit() */
	ldr r1, =z_arm_int_exit
	bx r1
