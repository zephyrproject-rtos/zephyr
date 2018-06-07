/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/kernel_event_logger.h>

/*
 * @brief Put the CPU in low-power mode
 *
 * This function always exits with interrupts unlocked.
 *
 * void k_cpu_idle(void)
 */
void k_cpu_idle(void)
{
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	_sys_k_event_logger_enter_sleep();
#endif
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
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	_sys_k_event_logger_enter_sleep();
#endif
	__asm__ volatile ("waiti 0\n\t"
			  "wsr.ps %0\n\t"
			  "rsync" :: "a"(key));
}
