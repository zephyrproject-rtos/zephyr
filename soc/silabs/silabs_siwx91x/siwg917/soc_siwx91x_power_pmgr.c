/*
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include "sl_si91x_power_manager.h"
#include "sli_si91x_clock_manager.h"
#include "sl_rsi_utility.h"
#include "sl_si91x_m4_ps.h"

LOG_MODULE_REGISTER(siwx91x_pm);

extern uint32_t frontend_switch_control;

/*
 * Power state map:
 * PM_STATE_RUNTIME_IDLE: SL_SI91X_POWER_MANAGER_STANDBY (PS4)
 * PM_STATE_SUSPEND_TO_IDLE: SL_SI91X_POWER_MANAGER_SLEEP (PS4 Sleep)
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0. */
	irq_unlock(0);

	if (!sl_si91x_power_manager_is_ok_to_sleep()) {
		/* Device is not ready to sleep; perform necessary actions if required. */
		goto out;
	}

	if (state == PM_STATE_RUNTIME_IDLE) {
		sl_si91x_power_manager_standby();
	} else {
		if (sli_si91x_config_clocks_to_mhz_rc() != 0) {
			LOG_ERR("Failed to configure clocks for sleep mode");
			goto out;
		}
		if (IS_ENABLED(CONFIG_SILABS_SIWX91X_NWP)) {
			if (!(M4_ULP_SLP_STATUS_REG & ULP_MODE_SWITCHED_NPSS)) {
				if (!sl_si91x_is_device_initialized()) {
					LOG_ERR("Device is not initialized");
					goto out;
				}
				sli_si91x_xtal_turn_off_request_from_m4_to_TA();
			}
		}
		sl_si91x_power_manager_sleep();
	}

	if (!(M4_ULP_SLP_STATUS_REG & ULP_MODE_SWITCHED_NPSS)) {
		if (frontend_switch_control != 0) {
			sli_si91x_configure_wireless_frontend_controls(frontend_switch_control);
		}

		sl_si91x_host_clear_sleep_indicator();
		sli_si91x_m4_ta_wakeup_configurations();
	}
out:
	/* Clear PRIMASK */
	__enable_irq();
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);
}
