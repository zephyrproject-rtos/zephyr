/*
 * Copyright (c) 2021, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/arch/arch_interface.h>
#include <fsl_power.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/*!< Power down all unnecessary blocks and enable RBB during deep sleep. */
#define APP_DEEPSLEEP_RUNCFG0 (SYSCTL0_PDRUNCFG0_LPOSC_PD_MASK | \
			SYSCTL0_PDSLEEPCFG0_RBB_PD_MASK)
#define APP_DEEPSLEEP_RAM_APD 0xFFFFF8U
#define APP_DEEPSLEEP_RAM_PPD 0x0U
#define APP_EXCLUDE_FROM_DEEPSLEEP			\
	(((const uint32_t[]){APP_DEEPSLEEP_RUNCFG0, \
	(SYSCTL0_PDSLEEPCFG1_FLEXSPI_SRAM_APD_MASK | \
	 SYSCTL0_PDSLEEPCFG1_FLEXSPI_SRAM_PPD_MASK), \
	APP_DEEPSLEEP_RAM_APD, APP_DEEPSLEEP_RAM_PPD}))

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	unsigned int key;

	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		key = arch_pm_state_set_prepare();
		POWER_EnterSleep();
		arch_pm_state_set_finish(key);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		key = arch_pm_state_set_prepare();
		POWER_EnterDeepSleep(APP_EXCLUDE_FROM_DEEPSLEEP);
		arch_pm_state_set_finish(key);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}
