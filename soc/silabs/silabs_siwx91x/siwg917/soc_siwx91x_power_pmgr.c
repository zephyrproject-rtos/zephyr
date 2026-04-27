/*
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include "siwx91x_nwp_bus.h"
#include "siwx91x_nwp.h"

#include "sl_si91x_power_manager.h"
#include "sli_si91x_clock_manager.h"
#include "sl_rsi_utility.h"
#include "sl_si91x_m4_ps.h"

LOG_MODULE_REGISTER(siwx91x_pm);

/*
 * Power state map:
 * PM_STATE_RUNTIME_IDLE: SL_SI91X_POWER_MANAGER_STANDBY (PS4)
 * PM_STATE_SUSPEND_TO_IDLE: SL_SI91X_POWER_MANAGER_SLEEP (PS4 Sleep)
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	const struct device *dev_nwp = DEVICE_DT_GET(DT_NODELABEL(nwp));
	int ret;

	if (state == PM_STATE_RUNTIME_IDLE) {
		arch_cpu_idle();
		return;
	}
	if (!device_is_ready(dev_nwp)) {
		arch_cpu_idle();
		return;
	}
	if (sl_si91x_get_lowest_ps() != SL_SI91X_POWER_MANAGER_SLEEP) {
		arch_cpu_idle();
		return;
	}
	ret = siwx91x_nwp_prepare_sleep(dev_nwp);
	if (ret) {
		arch_cpu_idle();
		return;
	}
	ret = sli_si91x_config_clocks_to_mhz_rc();
	if (ret) {
		arch_cpu_idle();
		goto fail_clock;
	}
	siwx91x_nwp_enter_sleep(dev_nwp);
	__disable_irq();
	irq_unlock(0);
	ret = sl_si91x_power_manager_sleep();
	__enable_irq();
	__ASSERT(!ret, "sl_si91x_power_manager_sleep: Failure: %d", ret);
fail_clock:
	siwx91x_nwp_exit_sleep(dev_nwp);
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
}
