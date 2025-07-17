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

#define WAKEUP_DELAY         0x30C
#define WAKEUP_COUNTER_INDEX 6
#define WAKEUP_BUTTON_INDEX  4

void z_sys_poweroff(void)
{
	cmc_power_domain_config_t cmc_config = {
		.clock_mode = kCMC_GateAllSystemClocksEnterLowPowerMode,
		.main_domain = kCMC_DeepPowerDown,
		.wake_domain = kCMC_DeepPowerDown,
	};

#if defined(CONFIG_SOC_MCXN947)
	WUU_SetInternalWakeUpModulesConfig(WUU0, WAKEUP_COUNTER_INDEX,
					   kWUU_InternalModuleInterrupt);
#endif

#if defined(CONFIG_SOC_MCXN236)
	wuu_external_wakeup_pin_config_t wakeupButtonConfig;

	wakeupButtonConfig.edge = kWUU_ExternalPinFallingEdge;
	wakeupButtonConfig.event = kWUU_ExternalPinInterrupt;
	wakeupButtonConfig.mode = kWUU_ExternalPinActiveAlways;
	if (WUU_GetExternalWakeUpPinsFlag(WUU0) == BIT(WAKEUP_BUTTON_INDEX)) {
		WUU_ClearExternalWakeUpPinsFlag(WUU0, BIT(WAKEUP_BUTTON_INDEX));
	}

	WUU_SetExternalWakeUpPinsConfig(WUU0, WAKEUP_BUTTON_INDEX, &wakeupButtonConfig);
#endif

	SPC_SetLowPowerWakeUpDelay(SPC0, WAKEUP_DELAY);

	CMC_SetPowerModeProtection(CMC0, kCMC_AllowDeepPowerDownMode);
	CMC_EnterLowPowerMode(CMC0, &cmc_config);

	CODE_UNREACHABLE;
}
