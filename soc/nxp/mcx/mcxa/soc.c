/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxa platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxa platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
#include <fsl_spc.h>
#include <fsl_cmc.h>
#define MCXA_CMC	((CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc)))
#define MCXA_SPC	((SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc)))
#endif

#ifdef CONFIG_SOC_RESET_HOOK
void soc_reset_hook(void)
{
#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
	if ((CMC_GetSystemResetStatus(MCXA_CMC) & kCMC_WakeUpReset) != 0UL) {
		SPC_ClearPeriphIOIsolationFlag(MCXA_SPC);
		SPC_ClearPowerDomainLowPowerRequestFlag(MCXA_SPC, kSPC_PowerDomain0);
		SPC_ClearLowPowerRequest(MCXA_SPC);
	}
#endif
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	SystemInit();
#endif /* ! CONFIG_TRUSTED_EXECUTION_NONSECURE */
}
#endif

void enable_ecc(uint32_t mask)
{
	SYSCON->RAM_CTRL = mask;
}
