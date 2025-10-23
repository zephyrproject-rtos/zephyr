/*
 * Copyright (c) 2024-2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>

#include <mxc_device.h>
#include <wrap_max32_lp.h>

#include <zephyr/logging/log.h>
#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("entering PM state runtime idle");
		MXC_LP_EnterSleepMode();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("entering PM state suspend to idle");
		Wrap_MXC_LP_EnterMicroPowerMode();
		break;
	case PM_STATE_STANDBY:
		LOG_DBG("entering PM state standby");
		Wrap_MXC_LP_EnterStandbyMode();
		break;
	case PM_STATE_SOFT_OFF:
		LOG_DBG("entering PM state powerdown");
		Wrap_MXC_LP_EnterPowerDownMode();
		/*Code not reach here */
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}
	LOG_DBG("wakeup from sleep mode");
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Set run mode config after wakeup */
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("exited PM state runtime idle");
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("exited PM state suspend to idle");
		break;
	case PM_STATE_STANDBY:
		LOG_DBG("exited PM state standby");
		break;
	default:
		break;
	}

	/* Clear PRIMASK after wakeup */
	__enable_irq();
}
