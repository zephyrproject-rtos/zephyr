/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/device.h>
#include <r_lpm.h>

/* Private headers in zephyr/drivers/... */
#include <wuc/wuc_renesas_ra_lpm_priv.h>

void z_sys_poweroff(void)
{
	static struct st_lpm_cfg cfg = {
#if DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, capabilities, has_deep_software_standby) &&             \
	DT_ENUM_HAS_VALUE(RENESAS_RA_LPM_NODE, deep_standby_options, io_port_keep)
		.io_port_state  = LPM_IO_PORT_NO_CHANGE,
#else
		.io_port_state  = LPM_IO_PORT_RESET,
#endif
		.low_power_mode = LPM_MODE_DEEP,
		/* Use deepest supply mode */
		.power_supply_state = LPM_POWER_SUPPLY_DEEP_STANDBY_MODE3,
		.p_extend       = NULL,
	};

	static struct st_lpm_instance_ctrl ctrl;
#ifdef CONFIG_WUC_RENESAS_RA_LPM
	const struct device *wuc = DEVICE_DT_GET(DT_NODELABEL(wuc));

	if (device_is_ready(wuc)) {
		cfg.deep_standby_cancel_source = ra_wuc_get_cancel_source(wuc);
		cfg.deep_standby_cancel_edge   = ra_wuc_get_cancel_edge(wuc);
	}
#endif
	(void)irq_lock();

	R_LPM_Open(&ctrl, &cfg);
	R_LPM_LowPowerModeEnter(&ctrl);

	CODE_UNREACHABLE;
}
