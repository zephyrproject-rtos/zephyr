/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/tie/xt_core.h>
#include <xtensa/tie/xt_interrupt.h>
#include <tracing.h>

/*
 * @brief Put the CPU in low-power mode
 *
 * This function always exits with interrupts unlocked.
 *
 * void k_cpu_idle(void)
 */
void k_cpu_idle(void)
{
	sys_trace_idle();
	__asm__ volatile ("waiti 0");
}
/*
 * @brief Put the CPU in low-power mode, entered with IRQs locked
 *
 * This function exits with interrupts restored to <key>.
 *
 * void k_cpu_atomic_idle(unsigned int key)
 */
void k_cpu_atomic_idle(unsigned int key)
{
	sys_trace_idle();
	__asm__ volatile ("waiti 0\n\t"
			  "wsr.ps %0\n\t"
			  "rsync" :: "a"(key));
}
