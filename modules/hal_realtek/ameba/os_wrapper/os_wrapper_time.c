/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "os_wrapper.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(os_if_time);

void rtos_time_delay_ms(uint32_t ms)
{
	if (k_is_in_isr() || rtos_sched_get_state() == RTOS_SCHED_NOT_STARTED) {
		DelayMs(ms);
	} else {
		k_msleep(ms);
	}
}

void rtos_time_delay_us(uint32_t us)
{
	DelayUs(us);
}

uint32_t rtos_time_get_current_system_time_ms(void)
{
	return k_uptime_get_32();
}

uint64_t rtos_time_get_current_system_time_us(void)
{
	return k_ticks_to_us_floor64(sys_clock_tick_get());
}

uint64_t rtos_time_get_current_system_time_ns(void)
{
	return k_ticks_to_ns_floor64(sys_clock_tick_get());
}

uint32_t rtos_time_get_current_pended_time_ms(void)
{
	/* rtos_sched_suspend does not affect the value of rtos_time_get_current_system_time_ms */
	return 0;
}
