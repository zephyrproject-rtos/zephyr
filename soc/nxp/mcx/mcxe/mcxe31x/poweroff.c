/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <fsl_clock.h>
#include <fsl_power.h>
#include <fsl_wkpu.h>

void z_sys_poweroff(void)
{
	wkpu_external_wakeup_source_config_t wkpu_config;

	CLOCK_EnableSircInStandbyMode();
	CLOCK_EnableClock(kCLOCK_Wkup);
	WKPU_GetDefaultExternalWakeUpSourceConfig(&wkpu_config);
	WKPU_SetExternalWakeUpSourceConfig(WKPU, kWKPU_Source1, &wkpu_config);
	DCM_GPR->DCMRWF1 &= ~((uint32_t)DCM_GPR_DCMRWF1_STANDBY_IO_CONFIG_MASK);

	POWER_EnableAutoTurnOverLastMileRegulator();
	POWER_DisableLastMileRegulator();
	POWER_DisableLPMV25Regulator();

	POWER_EnterStandbyMode();

	CODE_UNREACHABLE;
}
