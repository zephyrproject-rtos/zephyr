/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>
#include <hal/nrf_resetinfo.h>
#include "haltium_pm_s2ram.h"
#include "haltium_power.h"

#include <cmsis_core.h>

int soc_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	int ret;

	ret = arch_pm_s2ram_suspend(system_off);

	/* Cache and FPU are powered down so power up is needed even if s2ram failed. */
	nrf_power_up_cache();

	return ret;
}

void pm_s2ram_mark_set(void)
{
	/* empty */
}

bool pm_s2ram_mark_check_and_clear(void)
{
	bool restore_valid;
	uint32_t reset_reason = nrf_resetinfo_resetreas_local_get(NRF_RESETINFO);

	if (reset_reason != NRF_RESETINFO_RESETREAS_LOCAL_UNRETAINED_MASK) {
		return false;
	}
	nrf_resetinfo_resetreas_local_set(NRF_RESETINFO, 0);

	restore_valid = nrf_resetinfo_restore_valid_check(NRF_RESETINFO);
	nrf_resetinfo_restore_valid_set(NRF_RESETINFO, false);

	return restore_valid;
}
