/*
 * Copyright (c) 2025 Ambiq LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/init.h>

/* ambiq-sdk includes */
#include <am_mcu_apollo.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	__disable_irq();
	__set_BASEPRI(0);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE: {
		/* Put ARM core to normal sleep. */
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
		break;
	}
	case PM_STATE_SUSPEND_TO_RAM: {
		/* Put ARM core to deep sleep. */
		/* Cotex-m: power down, register value preserve.*/
		/* Cache: power down*/
		/* MRAM: power down*/
		/* ITCM + DTCM: retention, active on request*/
		/* Sram: retention, active on request*/
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
		break;
	}
	default: {
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
	}
}

/**
 * @brief PM State Exit Post Operations
 *
 * For PM_STATE_SUSPEND_TO_IDLE:
 *   Nothing is needed after soc woken up.
 *
 * For PM_STATE_SUSPEND_TO_RAM:
 *   Flash, cache, sram automatically switch
 *   to active state on wake up
 *
 * @param state PM State
 * @param substate_id Unused
 *
 */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	__enable_irq();
	irq_unlock(0);
}

void ambiq_power_init(void)
{
	am_hal_pwrctrl_mcu_memory_config_t McuMemCfg = {
		.eROMMode = AM_HAL_PWRCTRL_ROM_AUTO,
		.eDTCMCfg = AM_HAL_PWRCTRL_ITCM256K_DTCM512K,
		.eRetainDTCM = AM_HAL_PWRCTRL_MEMRETCFG_TCMPWDSLP_RETAIN,
		.eNVMCfg = AM_HAL_PWRCTRL_NVM0_AND_NVM1,
		.bKeepNVMOnInDeepSleep = false};

	am_hal_pwrctrl_sram_memcfg_t SRAMMemCfg = {.eSRAMCfg = AM_HAL_PWRCTRL_SRAM_3M,
						   .eActiveWithMCU = AM_HAL_PWRCTRL_SRAM_NONE,
						   .eActiveWithGFX = AM_HAL_PWRCTRL_SRAM_NONE,
						   .eActiveWithDISP = AM_HAL_PWRCTRL_SRAM_NONE,
						   .eSRAMRetain = AM_HAL_PWRCTRL_SRAM_3M};

	am_hal_status_e ret;

	ret = am_hal_pwrctrl_mcu_memory_config(&McuMemCfg);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("mcu_memory_config failed: 0x%08X", ret);
	}

	ret = am_hal_pwrctrl_sram_config(&SRAMMemCfg);
	if (ret != AM_HAL_STATUS_SUCCESS) {
		LOG_ERR("sram_config failed: 0x%08X", ret);
	}
}
