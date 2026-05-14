/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/counter.h>
#include <soc.h>
#include <r_lpm.h>

/* Private headers in zephyr/drivers/... */
#include <wuc/wuc_renesas_ra_lpm_priv.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define RENESAS_RA_LPM_NODE DT_NODELABEL(lpm)

#define RA_LPM_INCLUDE_ELEM(node_id, prop, idx)                                                    \
	| BIT64(DT_PROP_BY_IDX(node_id, prop, idx))

#define RA_LPM_STANDBY_SRC0                                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(RENESAS_RA_LPM_NODE, standby_wakeup_sources),                 \
	((lpm_standby_wake_source_t)(0ULL DT_FOREACH_PROP_ELEM(RENESAS_RA_LPM_NODE,                \
	standby_wakeup_sources, RA_LPM_INCLUDE_ELEM))),                                            \
	((lpm_standby_wake_source_t)(0xFFFFFFFFFFFFFFFFULL)))

#define RA_LPM_STANDBY_SRC2                                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(RENESAS_RA_LPM_NODE, standby_wakeup_sources_extended),        \
	((lpm_standby_wake_source_2_t)(0ULL DT_FOREACH_PROP_ELEM(RENESAS_RA_LPM_NODE,              \
	standby_wakeup_sources_extended, RA_LPM_INCLUDE_ELEM))),                                   \
	((lpm_standby_wake_source_2_t)(0xFFFFFFFFFFFFFFFFULL)))

#define RA_LPM_DSLP_IRQ_MASK_0                                                                     \
	DT_PROP_BY_IDX(RENESAS_RA_LPM_NODE, deep_sleep_wakeup_irq_mask, 0)

#define RA_LPM_DSLP_IRQ_MASK_1                                                                     \
	DT_PROP_BY_IDX(RENESAS_RA_LPM_NODE, deep_sleep_wakeup_irq_mask, 1)

#define RA_LPM_DSLP_IRQ_MASK_2                                                                     \
	DT_PROP_BY_IDX(RENESAS_RA_LPM_NODE, deep_sleep_wakeup_irq_mask, 2)

static struct st_lpm_cfg renesas_ra_lpm_fsp_cfg = {
	.standby_wake_sources = (lpm_standby_wake_source_t)(RA_LPM_STANDBY_SRC0),

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_extended_wakeup)
	.standby_wake_sources_2 = (lpm_standby_wake_source_2_t)(RA_LPM_STANDBY_SRC2),
#endif

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_output_port_retain)
	.output_port_enable =
		DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, standby_options, output_port_retain)
			? LPM_OUTPUT_PORT_ENABLE_RETAIN
			: LPM_OUTPUT_PORT_ENABLE_HIGH_IMPEDANCE,
#endif

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_ram_retain)
	.ram_retention_cfg.ram_retention =
		DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, standby_options, sram_retain)
			? (uint16_t)0xffff
			: (uint16_t)0,
	.ram_retention_cfg.tcm_retention =
		DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, standby_options, tcm_retain),
#endif

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_ldo_retain)
	.ldo_standby_cfg.pll1_ldo =
		DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, standby_options, pll1_ldo_retain)
			? LPM_LDO_STANDBY_OPERATION_RETAINED
			: LPM_LDO_STANDBY_OPERATION_DISABLED,
	.ldo_standby_cfg.pll2_ldo =
		DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, standby_options, pll2_ldo_retain)
			? LPM_LDO_STANDBY_OPERATION_RETAINED
			: LPM_LDO_STANDBY_OPERATION_DISABLED,
	.ldo_standby_cfg.hoco_ldo =
		DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, standby_options, hoco_ldo_retain)
			? LPM_LDO_STANDBY_OPERATION_RETAINED
			: LPM_LDO_STANDBY_OPERATION_DISABLED,
#endif

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_hoco_startup_speed)
	.lpm_hoco_startup_speed =
		DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, standby_options, hoco_high_speed_startup)
			? LPM_HOCO_STARTUP_SPEED_HIGH_SPEED
			: LPM_HOCO_STARTUP_SPEED_NORMAL_SPEED,
#endif

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_standby_sosc_select)
	.lpm_standby_sosc = DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, standby_options, sosc_retain)
				    ? LPM_STANDBY_SOSC_ENABLE
				    : LPM_STANDBY_SOSC_DISABLE,
#endif

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_deep_software_standby)
	.io_port_state  = DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, deep_standby_options, io_port_keep)
				? LPM_IO_PORT_NO_CHANGE
				: LPM_IO_PORT_RESET,
#endif

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_dpsbycr_dcssmode)
	.deep_standby_soft_start_mode =
			(lpm_deep_standby_soft_start_mode_t)
			DT_PROP(RENESAS_RA_LPM_NODE, deep_standby_soft_start_mode),
#endif

	.p_extend = NULL,
};

static struct st_lpm_instance_ctrl renesas_ra_lpm_fsp_ctrl;

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		ARG_UNUSED(substate_id);
		renesas_ra_lpm_fsp_cfg.low_power_mode = LPM_MODE_SLEEP;
		break;

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_deep_sleep)
	case PM_STATE_SUSPEND_TO_IDLE:
		ARG_UNUSED(substate_id);
#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_deep_sleep_irq_wakeup)
		/* Configure interrupt sources that are allowed to wake the CPU from Deep Sleep
		 * mode if the device tree node has the "deep_sleep_wakeup_irq_mask" property,
		 * else using the interrupt sources configured through WUPEN registers.
		 */
		R_ICU->DSLPWUPIRQEN[0] = RA_LPM_DSLP_IRQ_MASK_0;
		R_ICU->DSLPWUPIRQEN[1] = RA_LPM_DSLP_IRQ_MASK_1;
		R_ICU->DSLPWUPIRQEN[2] = RA_LPM_DSLP_IRQ_MASK_2;
#endif
		renesas_ra_lpm_fsp_cfg.low_power_mode = LPM_MODE_DEEP_SLEEP;
		break;
#endif

	case PM_STATE_STANDBY:
		ARG_UNUSED(substate_id);
		renesas_ra_lpm_fsp_cfg.low_power_mode = LPM_MODE_STANDBY;
		break;

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_deep_software_standby)
	case PM_STATE_SOFT_OFF: {
#ifdef CONFIG_WUC_RENESAS_RA_LPM
		/* Forward the deep standby wakeup sources to the FSP LPM configuration. */
		const struct device *wuc = DEVICE_DT_GET(DT_NODELABEL(wuc));

		if (device_is_ready(wuc)) {
			renesas_ra_lpm_fsp_cfg.deep_standby_cancel_source =
				ra_wuc_get_cancel_source(wuc);
			renesas_ra_lpm_fsp_cfg.deep_standby_cancel_edge =
				ra_wuc_get_cancel_edge(wuc);
		} else {
			LOG_WRN("WUC device not ready, deep standby wakeup sources disabled");
		}
#endif
		/* Configure the substate for deep standby mode */
		if (substate_id == 0) {
			renesas_ra_lpm_fsp_cfg.power_supply_state =
				LPM_POWER_SUPPLY_DEEP_STANDBY_MODE1;
		} else if (substate_id == 1) {
			renesas_ra_lpm_fsp_cfg.power_supply_state =
				LPM_POWER_SUPPLY_DEEP_STANDBY_MODE2;
		} else  {
			renesas_ra_lpm_fsp_cfg.power_supply_state =
				LPM_POWER_SUPPLY_DEEP_STANDBY_MODE3;
		}

		renesas_ra_lpm_fsp_cfg.low_power_mode = LPM_MODE_DEEP;
		break;
	}
#endif
	default:
		k_cpu_idle();
		return;
	}

	R_LPM_Open(&renesas_ra_lpm_fsp_ctrl, &renesas_ra_lpm_fsp_cfg);
	R_LPM_LowPowerModeEnter(&renesas_ra_lpm_fsp_ctrl);
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		__fallthrough;

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_deep_sleep)
	case PM_STATE_SUSPEND_TO_IDLE:
		__fallthrough;
#endif

	case PM_STATE_STANDBY:
		R_LPM_Close(&renesas_ra_lpm_fsp_ctrl);
		break;

#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_deep_software_standby)
	case PM_STATE_SOFT_OFF:
		/* Never reached - deep standby wakeup is a system reset */
		CODE_UNREACHABLE;
		break;
#endif
	default:
		break;
	}

	irq_unlock(0);
}

static int renesas_ra_idle_counter_start(void)
{
	const struct device *dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_system_timer_companion));

	if (dev) {
		return counter_start(dev);
	}

	LOG_WRN("No system timer companion device found");

	return -ENODEV;
}

SYS_INIT(renesas_ra_idle_counter_start, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
