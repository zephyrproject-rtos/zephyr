/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

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
	z_sys_trace_idle();
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
	z_sys_trace_idle();
	__asm__ volatile ("waiti 0\n\t"
			  "wsr.ps %0\n\t"
			  "rsync" :: "a"(key));
}
