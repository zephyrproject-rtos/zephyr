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

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_exc_exit)
GTEXT(z_arm_int_exit)
GTEXT(z_arm_pendsv)
GDATA(_kernel)

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

/* z_arm_int_exit falls through to z_arm_exc_exit (they are aliases of each
 * other)
 */

/**
 *
 * @brief Kernel housekeeping when exiting exception handler installed
 * 		directly in vector table
 *
 * See z_arm_int_exit().
 *
 * @return N/A
 */

SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, z_arm_exc_exit)

#ifdef CONFIG_PREEMPT_ENABLED
	/* Do not context switch if exiting a nested interrupt */
	ldr r3, =_kernel
	ldr r0, [r3, #_kernel_offset_to_nested]
	cmp r0, #1
	bhi _EXIT_EXC

	ldr r1, [r3, #_kernel_offset_to_current]
	ldr r0, [r3, #_kernel_offset_to_ready_q_cache]
	cmp r0, r1
	blne z_arm_pendsv
_EXIT_EXC:
#endif /* CONFIG_PREEMPT_ENABLED */

#ifdef CONFIG_STACK_SENTINEL
	bl z_check_stack_sentinel
#endif /* CONFIG_STACK_SENTINEL */

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
	 * Restore r0-r3, r12 and lr_irq stored into the process stack by the
	 * mode entry function. These registers are saved by _isr_wrapper for
	 * IRQ mode and z_arm_svc for SVC mode.
	 *
	 * r0-r3 are either the values from the thread before it was switched
	 * out or they are the args to _new_thread for a new thread.
	 */
	cps #MODE_SYS
	pop {r0-r3, r12, lr}
	rfeia sp!
