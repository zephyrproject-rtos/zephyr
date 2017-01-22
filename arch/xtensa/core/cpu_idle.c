/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/tie/xt_core.h>
#include <xtensa/tie/xt_interrupt.h>
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
	XT_WAITI(0);
}
/*
 * @brief Put the CPU in low-power mode, entered with IRQs locked
 *
 * This function exits with interrupts restored to <key>.
 *
 * void nano_cpu_atomic_idle(unsigned int key)
 */
void k_cpu_atomic_idle(unsigned int key)
{
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	_sys_k_event_logger_enter_sleep();
#endif
	XT_WAITI(0);
	XT_WSR_PS(key);
	XT_RSYNC();
}
