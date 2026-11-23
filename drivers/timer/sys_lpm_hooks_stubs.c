/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Stub implementation of certain functions of the system timer
 * low-power companion interface. These stubs are weak functions
 * so they can be overridden.
 */

#include <zephyr/drivers/timer/system_timer_lpm.h>
#include <zephyr/toolchain.h>

__weak void z_sys_clock_lpm_init(void)
{
	/* Stub implementation: do nothing */
}
