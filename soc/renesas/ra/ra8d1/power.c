/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include <zephyr/init.h>
#include <r_lpm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

lpm_instance_ctrl_t g_lpm_sleep_ctrl;
const lpm_cfg_t g_lpm_sleep_cfg = {
	.low_power_mode = LPM_MODE_SLEEP,
	.standby_wake_sources = (lpm_standby_wake_source_t)0,
#if BSP_FEATURE_LPM_HAS_SNOOZE
	.snooze_cancel_sources = RA_NOT_DEFINED,
	.snooze_request_source = RA_NOT_DEFINED,
	.snooze_end_sources = (lpm_snooze_end_t)0,
	.dtc_state_in_snooze = RA_NOT_DEFINED,
#endif
#if BSP_FEATURE_LPM_HAS_SBYCR_OPE
	.output_port_enable = LPM_OUTPUT_PORT_ENABLE_RETAIN,
#endif
#if BSP_FEATURE_LPM_HAS_DEEP_STANDBY
	.io_port_state = LPM_IO_PORT_NO_CHANGE,
	.power_supply_state = LPM_POWER_SUPPLY_DEEP_STANDBY_MODE1,
	.deep_standby_cancel_source = (lpm_deep_standby_cancel_source_t)0,
	.deep_standby_cancel_edge = (lpm_deep_standby_cancel_edge_t)0,
#endif
#if BSP_FEATURE_LPM_HAS_PDRAMSCR
	.ram_retention_cfg.ram_retention = (uint8_t)(0),
	.ram_retention_cfg.tcm_retention = true,
#endif
#if BSP_FEATURE_LPM_HAS_DPSBYCR_SRKEEP
	.ram_retention_cfg.standby_ram_retention = true,
#endif
#if BSP_FEATURE_LPM_HAS_LDO_CONTROL
	.ldo_standby_cfg.pll1_ldo = false,
	.ldo_standby_cfg.pll2_ldo = false,
	.ldo_standby_cfg.hoco_ldo = false,
#endif
	.p_extend = NULL,
};
lpm_instance_ctrl_t g_lpm_sw_standby_ctrl;
const lpm_cfg_t g_lpm_sw_standby_cfg = {
	.low_power_mode = LPM_MODE_STANDBY,
	.standby_wake_sources = LPM_STANDBY_WAKE_SOURCE_ULP0U | (lpm_standby_wake_source_t)0,
#if BSP_FEATURE_LPM_HAS_SNOOZE
	.snooze_cancel_sources = RA_NOT_DEFINED,
	.snooze_request_source = RA_NOT_DEFINED,
	.snooze_end_sources = (lpm_snooze_end_t)0,
	.dtc_state_in_snooze = RA_NOT_DEFINED,
#endif
#if BSP_FEATURE_LPM_HAS_SBYCR_OPE
	.output_port_enable = LPM_OUTPUT_PORT_ENABLE_RETAIN,
#endif
#if BSP_FEATURE_LPM_HAS_DEEP_STANDBY
	.io_port_state = LPM_IO_PORT_NO_CHANGE,
	.power_supply_state = LPM_POWER_SUPPLY_DEEP_STANDBY_MODE1,
	.deep_standby_cancel_source = (lpm_deep_standby_cancel_source_t)0,
	.deep_standby_cancel_edge = (lpm_deep_standby_cancel_edge_t)0,
#endif
#if BSP_FEATURE_LPM_HAS_PDRAMSCR
	.ram_retention_cfg.ram_retention = (uint8_t)((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) |
						     (1 << 4) | (1 << 5) | (1 << 6) | 0),
	.ram_retention_cfg.tcm_retention = true,
#endif
#if BSP_FEATURE_LPM_HAS_DPSBYCR_SRKEEP
	.ram_retention_cfg.standby_ram_retention = true,
#endif
#if BSP_FEATURE_LPM_HAS_LDO_CONTROL
	.ldo_standby_cfg.pll1_ldo = true,
	.ldo_standby_cfg.pll2_ldo = true,
	.ldo_standby_cfg.hoco_ldo = true,
#endif
	.p_extend = NULL,
};

/* Invoke Low Power/System Off specific Tasks */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	/* Set PRIMASK */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("power state %u", state);
		R_LPM_Open(&g_lpm_sleep_ctrl, &g_lpm_sleep_cfg);
		R_LPM_LowPowerModeEnter(&g_lpm_sleep_ctrl);
		break;
	case PM_STATE_STANDBY:
		R_LPM_Open(&g_lpm_sw_standby_ctrl, &g_lpm_sw_standby_cfg);
		R_LPM_LowPowerModeEnter(&g_lpm_sw_standby_ctrl);
		LOG_DBG("power state %u", state);
		break;
	case PM_STATE_SOFT_OFF:
		LOG_DBG("power state %u", state);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		return;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		R_LPM_Close(&g_lpm_sleep_ctrl);
		break;
	case PM_STATE_STANDBY:
		R_LPM_Close(&g_lpm_sw_standby_ctrl);
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
	__enable_irq();
}

/* Initialize RA8 Power */
void ra_power_init(void)
{
#ifdef CONFIG_DEBUG
	/* Enable the Debug Module during STOP mode */
	R_SYSTEM->SYOCDCR_b.DBGEN = 1;
#endif /* CONFIG_DEBUG */
}
