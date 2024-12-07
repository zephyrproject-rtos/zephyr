/*
 * Copyright 2022, 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/pm/pm.h>

#include <fsl_clock.h>
#include <fsl_power.h>
#include "fsl_io_mux.h"

static void frdm_rw612_power_init_config(void)
{
	power_init_config_t initCfg = {
		/* VCORE AVDD18 supplied from iBuck on FRDM board. */
		.iBuck = true,
		/* CAU_SOC_SLP_REF_CLK needed for LPOSC. */
		.gateCauRefClk = false,
	};

	POWER_InitPowerConfig(&initCfg);
}

#if CONFIG_PM
static void frdm_rw612_pm_state_exit(enum pm_state state)
{
	switch (state) {
	case PM_STATE_STANDBY:
		frdm_rw612_power_init_config();
		break;
	default:
		break;
	}
}
#endif

static int frdm_rw612_init(void)
{
#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Eventually this code should not be here      */
	/* but should be configured by some SYSCTL node */

	/* Set shared signal set 0 SCK, WS from Transmit I2S - Flexcomm1 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(1) |
				SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(1);

	/* Select Data in from Transmit I2S - Flexcomm 1 */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_SHAREDDATASEL(1);
	/* Enable Transmit I2S - Flexcomm 1 for Shared Data Out */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_FC1DATAOUTEN(1);

	/* Set Receive I2S - Flexcomm 0 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[0] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

	/* Set Transmit I2S - Flexcomm 1 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

	/* Select Receive I2S - Flexcomm 0 Data in from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[0] |= SYSCTL1_FCCTRLSEL_DATAINSEL(1);
	/* Select Transmit I2S - Flexcomm 1 Data out to shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] |= SYSCTL1_FCCTRLSEL_DATAOUTSEL(1);

#endif /* CONFIG_I2S_TEST_SEPARATE_DEVICES */

	frdm_rw612_power_init_config();

#if CONFIG_PM
	static struct pm_notifier frdm_rw612_pm_notifier = {
		.state_exit = frdm_rw612_pm_state_exit,
	};

	/* clk_32k not derived from cau. It's safe to disable CAU clock in Power Mode 3, 4. */
	POWER_ConfigCauInSleep(true);

	pm_notifier_register(&frdm_rw612_pm_notifier);
#endif

	return 0;
}

SYS_INIT(frdm_rw612_init, PRE_KERNEL_1, CONFIG_BOARD_INIT_PRIORITY);
