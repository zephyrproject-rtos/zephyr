/*
 * Copyright 2022, 2024-25 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <fsl_power.h>

static void rdrw61x_power_init_config(void)
{
	power_init_config_t initCfg = {
		/* VCORE AVDD18 supplied from iBuck on RD board. */
		.iBuck = true,
		/* CAU_SOC_SLP_REF_CLK is needed for LPOSC. */
		.gateCauRefClk = false,
	};

	POWER_InitPowerConfig(&initCfg);
}

#if CONFIG_PM
static void rdrw61x_pm_state_exit(enum pm_state state)
{
	switch (state) {
	case PM_STATE_STANDBY:
		rdrw61x_power_init_config();
		break;
	default:
		break;
	}
}
#endif

void board_early_init_hook(void)
{
	rdrw61x_power_init_config();

#if CONFIG_PM
	static struct pm_notifier rdrw61x_pm_notifier = {
		.state_exit = rdrw61x_pm_state_exit,
	};

	pm_notifier_register(&rdrw61x_pm_notifier);
#endif
}
