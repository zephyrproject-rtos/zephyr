/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <hal/nrf_power.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SOFT_OFF:
		nrf_power_system_off(NRF_POWER);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SOFT_OFF:
		/* Nothing to do. */
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}

	/*
	 * System is now in active mode. Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}
