/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/internal/syscall_handler.h>

unsigned int z_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
void __weak sys_clock_notify_freq_change(uint32_t old_hz, uint32_t new_hz)
{
	ARG_UNUSED(old_hz);
	ARG_UNUSED(new_hz);
}

void z_sys_clock_hw_cycles_per_sec_update(uint32_t new_hz)
{
	unsigned int key;
	uint32_t old_hz;

	if (new_hz == 0U) {
		return;
	}

	key = irq_lock();
	old_hz = z_clock_hw_cycles_per_sec;
	z_clock_hw_cycles_per_sec = new_hz;
	irq_unlock(key);

	if (old_hz != new_hz) {
		sys_clock_notify_freq_change(old_hz, new_hz);
	}
}
#endif /* CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE */

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
#ifdef CONFIG_USERSPACE
static inline unsigned int z_vrfy_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	return z_impl_sys_clock_hw_cycles_per_sec_runtime_get();
}
#include <zephyr/syscalls/sys_clock_hw_cycles_per_sec_runtime_get_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */
