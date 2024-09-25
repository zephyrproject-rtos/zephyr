/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M exception/interrupt exit API
 *
 * Provides functions for performing kernel handling when exiting exceptions or
 * interrupts that are installed directly in the vector table (i.e. that are not
 * wrapped around by _isr_wrapper()).
 */

#include <zephyr/kernel.h>
#include <kswap.h>
#include <cmsis_core.h>

/**
 *
 * @brief Kernel housekeeping when exiting interrupt handler installed
 * 		directly in vector table
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
 *
 */
FUNC_ALIAS(z_arm_exc_exit, z_arm_int_exit, void);

/**
 *
 * @brief Kernel housekeeping when exiting exception handler installed
 * 		directly in vector table
 *
 * See z_arm_int_exit().
 *
 */
Z_GENERIC_SECTION(.text._HandlerModeExit) void z_arm_exc_exit(void)
{
#ifdef CONFIG_PREEMPT_ENABLED
	/* If thread is preemptible */
	if (_kernel.cpus->current->base.prio >= 0) {
		/* and cached thread is not current thread */
		if (_kernel.ready_q.cache != _kernel.cpus->current) {
			/* trigger a context switch */
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	}
#endif /* CONFIG_PREEMPT_ENABLED */

#ifdef CONFIG_STACK_SENTINEL
	z_check_stack_sentinel();
#endif /* CONFIG_STACK_SENTINEL */
}
