/*
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include "sl_si91x_power_manager.h"
#include "sli_si91x_clock_manager.h"
#include "sli_siwx917_soc.h"
#include "sl_rsi_utility.h"
#include "sl_si91x_m4_ps.h"

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

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


	if (state == PM_STATE_RUNTIME_IDLE) {
		sl_si91x_power_manager_standby();
	} else {
		if (!sl_si91x_power_manager_is_ok_to_sleep()) {
			/* Deep-sleep path is not ready yet; skip this suspend attempt. */
			goto out;
		}

		if (sli_si91x_config_clocks_to_mhz_rc() != 0) {
			LOG_ERR("Failed to configure clocks for sleep mode");
			goto out;
		}

		if (!(M4_ULP_SLP_STATUS_REG & ULP_MODE_SWITCHED_NPSS)) {
			if (!sl_si91x_is_device_initialized()) {
				LOG_ERR("Device is not initialized");
				goto out;
			}
			sli_si91x_xtal_turn_off_request_from_m4_to_TA();
		}

		sl_si91x_power_manager_sleep();

		if (!(M4_ULP_SLP_STATUS_REG & ULP_MODE_SWITCHED_NPSS)) {
			if (frontend_switch_control != 0) {
				sli_si91x_configure_wireless_frontend_controls(
					frontend_switch_control);
			}

			sl_si91x_host_clear_sleep_indicator();
			sli_si91x_m4_ta_wakeup_configurations();

		}
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

void siwx91x_power_init(void)
{
	sl_power_peripheral_t peripheral_config = {};
	sl_power_ram_retention_config_t ram_configuration = {
		.configure_ram_banks = false,
		/* The real RAM size from DT is correct but the power manager API expect to
		 * receive either 192/256/320 KB value, so we need to round it up to the
		 * next boundary to avoid powering down the top M4 bank(s) by mistake.
		 */
		.m4ss_ram_size_kb = (DT_REG_SIZE(DT_NODELABEL(sram0)) / 1024) + 1,
		.ulpss_ram_size_kb = 4,
	};

	sli_si91x_platform_init();
	sl_si91x_power_manager_init();
	sl_si91x_power_manager_remove_peripheral_requirement(&peripheral_config);
	sl_si91x_power_manager_configure_ram_retention(&ram_configuration);
	sl_si91x_power_manager_add_ps_requirement(SL_SI91X_POWER_MANAGER_PS4);
	sl_si91x_power_manager_set_clock_scaling(SL_SI91X_POWER_MANAGER_PERFORMANCE);
}
