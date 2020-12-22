/*
 * Copyright (c) 2017 Intel Corporation.
 * Copytight (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>

#ifdef CONFIG_HAS_POWER_STATE_DEEP_SLEEP_1
#include <hal/nrf_regulators.h>
#endif

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_PM_DEEP_SLEEP_STATES
 #ifdef CONFIG_HAS_POWER_STATE_DEEP_SLEEP_1
	case POWER_STATE_DEEP_SLEEP_1:
		nrf_regulators_system_off(NRF_REGULATORS);
		break;
 #endif
#endif
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(enum power_states state)
{
	switch (state) {
#ifdef CONFIG_PM_DEEP_SLEEP_STATES
 #ifdef CONFIG_HAS_POWER_STATE_DEEP_SLEEP_1
	case POWER_STATE_DEEP_SLEEP_1:
		/* Nothing to do. */
		break;
 #endif
#endif
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
