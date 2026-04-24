/*
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include "siwx91x_nwp.h"

#include "device/silabs/si91x/mcu/drivers/service/power_manager/inc/sl_si91x_power_manager.h"
#include "device/silabs/si91x/mcu/drivers/service/clock_manager/inc/sli_si91x_clock_manager.h"

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

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
		k_cpu_idle();
		return;
	}
	if (!device_is_ready(dev_nwp)) {
		k_cpu_idle();
		return;
	}
	__disable_irq();
	__set_BASEPRI(0);
	ret = siwx91x_nwp_prepare_sleep(dev_nwp);
	if (ret) {
		ret = 0;
		k_cpu_idle();
		goto fail_prepare;
	}
	ret = sli_si91x_config_clocks_to_mhz_rc();
	if (ret) {
		ret = 0;
		k_cpu_idle();
		goto fail_clock;
	}
	siwx91x_nwp_enter_sleep(dev_nwp);
	ret = sl_si91x_power_manager_sleep();
fail_clock:
	siwx91x_nwp_exit_sleep(dev_nwp);
fail_prepare:
	__enable_irq();
	__ASSERT(!ret, "sl_si91x_power_manager_sleep: Failure: %d", ret);
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
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

	RSI_Set_Cntrls_To_M4();
	sl_si91x_power_manager_init();
	sl_si91x_power_manager_remove_peripheral_requirement(&peripheral_config);
	sl_si91x_power_manager_configure_ram_retention(&ram_configuration);
	sl_si91x_power_manager_add_ps_requirement(SL_SI91X_POWER_MANAGER_PS4);
	sl_si91x_power_manager_set_clock_scaling(SL_SI91X_POWER_MANAGER_PERFORMANCE);
}
