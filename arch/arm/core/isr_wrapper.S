/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M wrapper for ISRs with parameter
 *
 * Wrapper installed in vector table for handling dynamic interrupts that accept
 * a parameter.
 */

#include <offsets_short.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <sw_isr_table.h>
#include <kernel_structs.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GDATA(_sw_isr_table)

GTEXT(_isr_wrapper)
GTEXT(_IntExit)

/**
 *
 * @brief Wrapper around ISRs when inserted in software ISR table
 *
 * When inserted in the vector table, _isr_wrapper() demuxes the ISR table using
 * the running interrupt number as the index, and invokes the registered ISR
 * with its corresponding argument. When returning from the ISR, it determines
 * if a context switch needs to happen (see documentation for __pendsv()) and
 * pends the PendSV exception if so: the latter will perform the context switch
 * itself.
 *
 * @return N/A
 */
SECTION_FUNC(TEXT, _isr_wrapper)

	push {lr}		/* lr is now the first item on the stack */

#ifdef CONFIG_EXECUTION_BENCHMARKING
	bl read_timer_start_of_isr
#endif

#ifdef CONFIG_TRACING
	bl z_sys_trace_isr_enter
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT
	/*
	 * All interrupts are disabled when handling idle wakeup.  For tickless
	 * idle, this ensures that the calculation and programming of the device
	 * for the next timer deadline is not interrupted.  For non-tickless idle,
	 * this ensures that the clearing of the kernel idle state is not
	 * interrupted.  In each case, _sys_power_save_idle_exit is called with
	 * interrupts disabled.
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
	blx _sys_power_save_idle_exit
_idle_state_cleared:

#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	ittt ne
	movne	r1, #0
		/* clear kernel idle state */
		strne	r1, [r2, #_kernel_offset_to_idle]
		blxne	_sys_power_save_idle_exit
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	cpsie i		/* re-enable interrupts (PRIMASK = 0) */
#endif

	mrs r0, IPSR	/* get exception number */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	ldr r1, =16
	subs r0, r1	/* get IRQ number */
	lsls r0, #3	/* table is 8-byte wide */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	sub r0, r0, #16	/* get IRQ number */
	lsl r0, r0, #3	/* table is 8-byte wide */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	ldr r1, =_sw_isr_table
	add r1, r1, r0	/* table entry: ISRs must have their MSB set to stay
			 * in thumb mode */

	ldm r1!,{r0,r3}	/* arg in r0, ISR in r3 */
#ifdef CONFIG_EXECUTION_BENCHMARKING
	stm sp!,{r0-r3} /* Save r0 to r4 into stack */
	push {lr}
	bl read_timer_end_of_isr
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop {r3}
	mov lr,r3
#else
	pop {lr}
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	ldm sp!,{r0-r3} /* Restore r0 to r4 regs */
#endif
	blx r3		/* call ISR */

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	pop {r3}
	mov lr, r3
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	pop {lr}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	/* exception return is done in _IntExit() */
	b _IntExit
