/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <sl_wifi.h>
#include <sl_si91x_ble.h>
#include <sl_si91x_power_manager.h>

#include "siwx91x_poweroff.h"

LOG_MODULE_REGISTER(siwx91x_poweroff, CONFIG_SOC_LOG_LEVEL);

int siwx91x_nwp_prepare_poweroff(void)
{

	sl_wifi_performance_profile_v2_t wifi_profile = {
		.profile = DEEP_SLEEP_WITHOUT_RAM_RETENTION,
	};
	sl_bt_performance_profile_t bt_profile = {
		.profile = DEEP_SLEEP_WITHOUT_RAM_RETENTION,
	};
	sl_status_t status;

	__ASSERT(!k_is_in_isr(), "NWP commands require thread context");

	/* The BT half must be pushed first. WiseConnect resolves the combined
	 * coex profile from both halves, so setting Wi-Fi first would make the
	 * combined profile reach deep sleep and invalidate the device before
	 * the BT request could be sent.
	 */
	if (IS_ENABLED(CONFIG_BT_SILABS_SIWX91X)) {
		status = sl_si91x_bt_set_performance_profile(&bt_profile);
		LOG_DBG("BT profile request: 0x%x\n", status);
		if (status != SL_STATUS_OK) {
			LOG_ERR("BT shutdown profile rejected: 0x%x", status);
			return status == SL_STATUS_NOT_INITIALIZED ? -ENODEV : -EIO;
		}
	}

	LOG_DBG("Wi-Fi profile request: 0x%x", status);
	status = sl_wifi_set_performance_profile_v2(&wifi_profile);
	LOG_DBG("Wi-Fi profile request: 0x%x", status);
	if (status != SL_STATUS_OK) {
		LOG_ERR("Wi-Fi shutdown profile rejected: 0x%x", status);
		return status == SL_STATUS_NOT_INITIALIZED ? -ENODEV : -EIO;
	}

	return status;
}

FUNC_NORETURN void z_sys_poweroff(void)
{
	sl_status_t status;

	for (sl_power_state_t ps_req = SL_SI91X_POWER_MANAGER_PS4;
	     ps_req > SL_SI91X_POWER_MANAGER_PS0; ps_req--) {
		status = sl_si91x_power_manager_remove_ps_requirement(ps_req);

		/*
		 * Removing a state with no requirement returns
		 * SL_STATUS_INVALID_PARAMETER. That is expected when clearing
		 * the state range. Other failures leave power-off unreliable.
		 */
		if ((status != SL_STATUS_OK) && (status != SL_STATUS_INVALID_PARAMETER)) {
			LOG_ERR("Failed to clear PS%d requirement: 0x%x", ps_req, status);
			k_panic();
			CODE_UNREACHABLE;
		}
	}

	status = sl_si91x_power_manager_add_ps_requirement(SL_SI91X_POWER_MANAGER_PS0);

	if (status != SL_STATUS_OK) {
		LOG_ERR("Failed to request PS0: 0x%x", status);
		k_panic();
	}

	/* z_sys_poweroff() has no error return path and must not return. */
	CODE_UNREACHABLE;
}
