/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/devicetree.h>
#include <soc.h>

#define SOC_MCX_WAKEUP_DELAY			\
	DT_PROP_OR(DT_NODELABEL(spc), wakeup_delay, 0)

#if CONFIG_SOC_FAMILY_MCXN
#define CMC_INSTANCE CMC0
#else
#define CMC_INSTANCE CMC
#endif

void z_sys_poweroff(void)
{
	cmc_power_domain_config_t cmc_config = {
		.clock_mode = kCMC_GateAllSystemClocksEnterLowPowerMode,
		.main_domain = kCMC_DeepPowerDown,
#if CONFIG_SOC_FAMILY_MCXN
		.wake_domain = kCMC_DeepPowerDown,
#endif
	};

	SPC_SetLowPowerWakeUpDelay(SPC0, SOC_MCX_WAKEUP_DELAY);

	CMC_SetPowerModeProtection(CMC_INSTANCE, kCMC_AllowDeepPowerDownMode);
	CMC_EnterLowPowerMode(CMC_INSTANCE, &cmc_config);

	CODE_UNREACHABLE;
}
