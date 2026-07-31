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
#include "soc_pm_s2ram.h"
#include "soc_power.h"

#include <cmsis_core.h>

#if DT_NODE_EXISTS(DT_NODELABEL(mcuboot_s2ram)) &&\
	DT_NODE_HAS_COMPAT(DT_NODELABEL(mcuboot_s2ram), zephyr_memory_region)
/* Linker section name is given by `zephyr,memory-region` property of
 * `zephyr,memory-region` compatible DT node with nodelabel `mcuboot_s2ram`.
 */
__attribute__((section(DT_PROP(DT_NODELABEL(mcuboot_s2ram), zephyr_memory_region))))
volatile struct mcuboot_resume_s _mcuboot_resume;

#define SET_MCUBOOT_RESUME_MAGIC() _mcuboot_resume.magic = MCUBOOT_S2RAM_RESUME_MAGIC
#else
#define SET_MCUBOOT_RESUME_MAGIC()
#endif

int soc_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	int ret;

	SET_MCUBOOT_RESUME_MAGIC();

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
