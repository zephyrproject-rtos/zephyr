/*
 * Copyright 2022, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/arch/arch_interface.h>
#include <fsl_power.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/*!< Power down all unnecessary blocks */
#define NODE_ID DT_INST(0, nxp_pdcfg_power)
#define EXCLUDE_FROM_DEEPSLEEP ((const uint32_t[]) \
					DT_PROP_OR(NODE_ID, deep_sleep_config, {}))

/* System clock frequency. */
extern uint32_t SystemCoreClock;

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
		POWER_EnterDeepSleep(EXCLUDE_FROM_DEEPSLEEP);
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

/* Initialize power system */
void rt5xx_power_init(void)
{
	/* This function is called to set vddcore low voltage detection
	 * falling trip voltage, this is not impacting the voltage in anyway.
	 */
	POWER_SetLdoVoltageForFreq(SystemCoreClock, 0);

#if CONFIG_REGULATOR
	/* Indicate to power library that PMIC is used. */
	POWER_UpdatePmicRecoveryTime(1);
#endif

}
