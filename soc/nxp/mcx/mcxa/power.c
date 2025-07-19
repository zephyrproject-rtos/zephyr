/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include "fsl_cmc.h"
#include "fsl_spc.h"
#include "fsl_vbat.h"
#include "fsl_wuu.h"

#define WAKEUP_DELAY         0x5B
#define WAKEUP_COUNTER_INDEX 6

void z_sys_poweroff(void)
{
	cmc_power_domain_config_t cmc_config = {
		.clock_mode = kCMC_GateAllSystemClocksEnterLowPowerMode,
		.main_domain = kCMC_DeepPowerDown,
	};

	WUU_SetInternalWakeUpModulesConfig(WUU0, WAKEUP_COUNTER_INDEX,
					   kWUU_InternalModuleInterrupt);

	SPC_SetLowPowerWakeUpDelay(SPC0, WAKEUP_DELAY);

	CMC_SetPowerModeProtection(CMC, kCMC_AllowDeepPowerDownMode);
	CMC_EnterLowPowerMode(CMC, &cmc_config);

	CODE_UNREACHABLE;
}
