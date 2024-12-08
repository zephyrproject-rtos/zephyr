/*
 * Copyright (c) 2024 Ambiq Micro Inc.
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
	case PM_STATE_SUSPEND_TO_IDLE:
		/* Put ARM core to normal sleep. */
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_NORMAL);
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		/* Put ARM core to deep sleep. */
		/* Cotex-m: power down, register value preserve.*/
		/* Cache: power down*/
		/* Flash: power down*/
		/* Sram: retention*/
		am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
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
	am_hal_pwrctrl_mcu_memory_config_t sMcuMemCfg = {
		.eCacheCfg    = AM_HAL_PWRCTRL_CACHE_NONE,
		.bRetainCache = true,
		.eDTCMCfg     = AM_HAL_PWRCTRL_DTCM_384K,
		.eRetainDTCM  = AM_HAL_PWRCTRL_DTCM_384K,
		.bEnableNVM0  = true,
		.bRetainNVM0  = false
	};

	am_hal_pwrctrl_sram_memcfg_t sSRAMCfg = {
		.eSRAMCfg        = AM_HAL_PWRCTRL_SRAM_ALL,
		.eActiveWithMCU	 = AM_HAL_PWRCTRL_SRAM_NONE,
		.eActiveWithGFX	 = AM_HAL_PWRCTRL_SRAM_NONE,
		.eActiveWithDISP = AM_HAL_PWRCTRL_SRAM_NONE,
		.eActiveWithDSP	 = AM_HAL_PWRCTRL_SRAM_NONE,
		.eSRAMRetain     = AM_HAL_PWRCTRL_SRAM_ALL
	};

	am_hal_pwrctrl_dsp_memory_config_t sDSPMemCfg = {
		.bEnableICache = false,
		.bRetainCache  = false,
		.bEnableRAM	   = false,
		.bActiveRAM	   = false,
		.bRetainRAM	   = false
	};

	am_hal_pwrctrl_mcu_memory_config(&sMcuMemCfg);
	am_hal_pwrctrl_sram_config(&sSRAMCfg);
	am_hal_pwrctrl_dsp_memory_config(AM_HAL_DSP0, &sDSPMemCfg);

	am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_PERIPH_CRYPTO);
}
