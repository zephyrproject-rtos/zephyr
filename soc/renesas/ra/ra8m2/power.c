/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/init.h>
#include <soc.h>
#include <r_lpm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/* Low Power Mode instance control structure */
static lpm_instance_ctrl_t pm_state_ctrl;

/* Configuration for Runtime Idle Power State */
const lpm_cfg_t pm_state_runtime_idle_cfg = {
	.low_power_mode = LPM_MODE_SLEEP,
	.standby_wake_sources = LPM_STANDBY_WAKE_SOURCE_ULP0U,
	.output_port_enable = LPM_OUTPUT_PORT_ENABLE_RETAIN,
	.io_port_state = LPM_IO_PORT_NO_CHANGE,
	.power_supply_state = LPM_POWER_SUPPLY_DEEP_STANDBY_MODE1,
	.deep_standby_cancel_source = (lpm_deep_standby_cancel_source_t)0,
	.deep_standby_cancel_edge = (lpm_deep_standby_cancel_edge_t)0,
	.ram_retention_cfg.ram_retention = (uint16_t)(0x7F),
	.ram_retention_cfg.tcm_retention = true,
	.ldo_standby_cfg.pll1_ldo = false,
	.ldo_standby_cfg.pll2_ldo = false,
	.ldo_standby_cfg.hoco_ldo = false,
	.p_extend = NULL,
};

/* Configuration for Standby Power State */
const lpm_cfg_t pm_state_standby_cfg = {
	.low_power_mode = LPM_MODE_STANDBY,
	.standby_wake_sources = LPM_STANDBY_WAKE_SOURCE_ULP0U,
	.output_port_enable = LPM_OUTPUT_PORT_ENABLE_RETAIN,
	.io_port_state = LPM_IO_PORT_NO_CHANGE,
	.power_supply_state = LPM_POWER_SUPPLY_DEEP_STANDBY_MODE1,
	.deep_standby_cancel_source = (lpm_deep_standby_cancel_source_t)0,
	.deep_standby_cancel_edge = (lpm_deep_standby_cancel_edge_t)0,
	.ram_retention_cfg.ram_retention = (uint16_t)(0x7F),
	.ram_retention_cfg.tcm_retention = true,
	.ldo_standby_cfg.pll1_ldo = false,
	.ldo_standby_cfg.pll2_ldo = false,
	.ldo_standby_cfg.hoco_ldo = false,
	.p_extend = NULL,
};

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		R_LPM_Open(&pm_state_ctrl, &pm_state_runtime_idle_cfg);
		__disable_irq();
		__set_BASEPRI(0);
		__ISB();

		R_LPM_LowPowerModeEnter(&pm_state_ctrl);
		__enable_irq();
		__ISB();
		break;

	case PM_STATE_STANDBY:
		R_LPM_Open(&pm_state_ctrl, &pm_state_standby_cfg);
		__disable_irq();
		__set_BASEPRI(0);
		__ISB();

		R_LPM_LowPowerModeEnter(&pm_state_ctrl);
		__enable_irq();
		__ISB();
		break;

	default:
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		__fallthrough;
	case PM_STATE_STANDBY:
		R_LPM_Close(&pm_state_ctrl);
		break;

	default:
		break;
	}
	irq_unlock(0);
}
