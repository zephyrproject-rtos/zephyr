/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/irq.h>
#include <zephyr/arch/cpu.h>

/**
 * @brief Power save idle routine
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of _pm_save_idle in the kernel when the
 * '_pm_save_flag' variable is non-zero.
 */
void arch_cpu_idle(void)
{
	/* curiously it arrives here with the interrupts masked
	 * so umask it before wait for an event
	 */
	arch_irq_unlock(MSTATUS_IEN);

	/* Wait for interrupt */
	__asm__ volatile("wfi");
}
