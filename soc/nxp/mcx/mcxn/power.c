/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include "fsl_cmc.h"
#include "fsl_spc.h"
#include "fsl_vbat.h"
#include "fsl_wuu.h"

void z_sys_poweroff(void)
{
    cmc_power_domain_config_t cmc_config = {
        .clock_mode  = kCMC_GateAllSystemClocksEnterLowPowerMode,
        .main_domain = kCMC_DeepPowerDown,
        .wake_domain = kCMC_DeepPowerDown,
    };

    WUU_SetInternalWakeUpModulesConfig(WUU0, 6U, kWUU_InternalModuleInterrupt);
    SPC_SetLowPowerWakeUpDelay(SPC0, 0x637);

    CMC_SetPowerModeProtection(CMC0, kCMC_AllowDeepPowerDownMode);
    CMC_EnterLowPowerMode(CMC0, &cmc_config);

    CODE_UNREACHABLE;
}
