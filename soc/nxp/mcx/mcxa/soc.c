/*
 * Copyright 2024-2026 NXP
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

#ifdef CONFIG_DT_HAS_NXP_SPC_ENABLED
#include <fsl_spc.h>
#define MCXA_SPC	((SPC_Type *)DT_REG_ADDR(DT_INST(0, nxp_spc)))
#endif

#ifdef CONFIG_SOC_RESET_HOOK

#if (defined(CONFIG_PM) || defined(CONFIG_POWEROFF)) && defined(CONFIG_DT_HAS_NXP_CMC_ENABLED)
#include <fsl_cmc.h>
#define MCXA_CMC	((CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc)))
#endif

void soc_reset_hook(void)
{
#if (defined(CONFIG_PM) || defined(CONFIG_POWEROFF)) && defined(CONFIG_DT_HAS_NXP_CMC_ENABLED) && \
	defined(CONFIG_DT_HAS_NXP_SPC_ENABLED)
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

#if defined(CONFIG_DT_HAS_NXP_SPC_ENABLED) && \
	DT_PROP(DT_INST(0, nxp_spc), active_mode_bandgap_buffer)
static int mcxa_spc_bandgap_init(void)
{
	/* The buffer drives the bandgap reference, so blocks outside
	 * the SPC (e.g. the LPADC bandgap input channel) can sample it.
	 */
	(void)SPC_SetActiveModeBandgapModeConfig(MCXA_SPC, kSPC_BandgapEnabledBufferEnabled);

	return 0;
}

SYS_INIT(mcxa_spc_bandgap_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

void enable_ecc(uint32_t mask)
{
	SYSCON->RAM_CTRL = mask;
}
