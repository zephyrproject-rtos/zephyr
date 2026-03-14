/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/soc/cpu_clock.h>
#include <zephyr/toolchain.h>

extern uint32_t SystemCoreClock;

#if defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#include <zephyr/drivers/timer/system_timer.h>
#endif

void z_soc_cpu_clock_hz_publish(uint32_t hz)
{
	if (hz == 0U) {
		return;
	}

	SystemCoreClock = hz;

#if defined(CONFIG_SOC_CPU_CLOCK_PUBLISH_UPDATE_SYSTEM_TIMER)
	z_sys_clock_hw_cycles_per_sec_update(hz);
#endif
}
