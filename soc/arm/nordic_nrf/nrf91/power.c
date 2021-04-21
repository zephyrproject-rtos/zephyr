/*
 * Copyright (c) 2017 Intel Corporation.
 * Copytight (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <hal/nrf_regulators.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(struct pm_state_info info)
{
	switch (info.state) {
	case PM_STATE_SOFT_OFF:
		nrf_regulators_system_off(NRF_REGULATORS);
		break;
	default:
		LOG_DBG("Unsupported power state %u", info.state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	switch (info.state) {
	case PM_STATE_SOFT_OFF:
		/* Nothing to do. */
		break;
	default:
		LOG_DBG("Unsupported power state %u", info.state);
		break;
	}

	/*
	 * System is now in active mode. Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}
