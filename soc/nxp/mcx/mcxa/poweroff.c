/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>

#define MCXA_CMC_ADDR		(CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))
#define MCXA_SPC_ADDR		(SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc))
#define MCXA_WAKEUP_DELAY	DT_PROP_OR(DT_INST(0, nxp_spc), wakeup_delay, 0)

void z_sys_poweroff(void)
{
	const cmc_power_domain_config_t cmc_config = {
		.clock_mode = kCMC_GateAllSystemClocksEnterLowPowerMode,
		.main_domain = kCMC_DeepPowerDown,
	};

	SPC_SetLowPowerWakeUpDelay(MCXA_SPC_ADDR, MCXA_WAKEUP_DELAY);
	CMC_SetPowerModeProtection(MCXA_CMC_ADDR, kCMC_AllowAllLowPowerModes);
	CMC_EnterLowPowerMode(MCXA_CMC_ADDR, &cmc_config);

	CODE_UNREACHABLE;
}
