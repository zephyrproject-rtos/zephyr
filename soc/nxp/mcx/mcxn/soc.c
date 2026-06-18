/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxn94x platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxn94x platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
#include <fsl_spc.h>
#include <fsl_cmc.h>
#endif

#ifdef CONFIG_SOC_EARLY_RESET_HOOK

void soc_early_reset_hook(void)
{
	/* SystemInit() disables RAM ECC, so the retained working set in RAMA
	 * is written without valid ECC check bits. The transparent S2RAM resume
	 * longjmps out of reset.S before soc_reset_hook / SystemInit() runs,
	 * while Deep Power Down has reset ECC_ENABLE_CTRL back to its hardware
	 * default (ECC on). So arch_pm_s2ram_resume() reads the retained marker
	 * with ECC erroneously on, mis-"corrects" it against the absent check
	 * bits and the marker check fails. Re-disable ECC here, before that read,
	 * restoring the SystemInit() state.
	 */
	SYSCON0->ECC_ENABLE_CTRL = 0U;
}

#endif /* CONFIG_SOC_EARLY_RESET_HOOK */

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
	if ((CMC_GetSystemResetStatus(CMC0) & kCMC_WakeUpReset) != 0UL) {
		SPC_ClearPeriphIOIsolationFlag(SPC0);
		SPC_ClearPowerDomainLowPowerRequestFlag(SPC0, kSPC_PowerDomain0);
		SPC_ClearPowerDomainLowPowerRequestFlag(SPC0, kSPC_PowerDomain1);
		SPC_ClearLowPowerRequest(SPC0);
	}
#endif
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	SystemInit();
#endif /* ! CONFIG_TRUSTED_EXECUTION_NONSECURE */
}

#endif

#define FLEXCOMM_CHECK_2(n)                                                                        \
	BUILD_ASSERT((DT_NODE_HAS_COMPAT(n, nxp_lpuart) == 0) &&                                   \
			     (DT_NODE_HAS_COMPAT(n, nxp_lpi2c) == 0),                              \
		     "Do not enable SPI and UART/I2C on the same Flexcomm node");

/* For SPI node enabled, check if UART or I2C is also enabled on the same parent Flexcomm node */
#define FLEXCOMM_CHECK(n) DT_FOREACH_CHILD_STATUS_OKAY(DT_PARENT(n), FLEXCOMM_CHECK_2)

/* SPI cannot be exist with UART or I2C on the same FlexComm Interface
 * Throw a build error if user is enabling SPI and UART/I2C on a Flexcomm node.
 */
DT_FOREACH_STATUS_OKAY(nxp_lpspi, FLEXCOMM_CHECK)

#if defined(CONFIG_SECOND_CORE_MCUX) &&                                                            \
	(defined(CONFIG_SOC_MCXN947_CPU0) || defined(CONFIG_SOC_MCXN547_CPU0))

/* This function is also called at deep sleep resume. */
static int second_core_boot(void)
{
	/* Configure CPU1 TrustZone access level before CPU1 is enabled */
	AHBSC->MASTER_SEC_LEVEL |=
		AHBSC_MASTER_SEC_LEVEL_CPU1(CONFIG_SECOND_CORE_MCUX_ACCESS_LEVEL);
	AHBSC->MASTER_SEC_ANTI_POL_REG =
		(~AHBSC->MASTER_SEC_LEVEL &
		 ~AHBSC_MASTER_SEC_ANTI_POL_REG_MASTER_SEC_LEVEL_ANTIPOL_LOCK_MASK) |
		AHBSC_MASTER_SEC_ANTI_POL_REG_MASTER_SEC_LEVEL_ANTIPOL_LOCK(2);

	/* Boot source for Core 1 from flash */
	SYSCON->CPBOOT = ((uint32_t)(char *)DT_REG_ADDR(DT_CHOSEN(zephyr_code_cpu1_partition)) &
			  SYSCON_CPBOOT_CPBOOT_MASK);

	uint32_t temp = SYSCON->CPUCTRL;

	temp |= 0xc0c40000U;
	SYSCON->CPUCTRL = temp | SYSCON_CPUCTRL_CPU1RSTEN_MASK | SYSCON_CPUCTRL_CPU1CLKEN_MASK;
	SYSCON->CPUCTRL = (temp | SYSCON_CPUCTRL_CPU1CLKEN_MASK) & (~SYSCON_CPUCTRL_CPU1RSTEN_MASK);

	return 0;
}

SYS_INIT(second_core_boot, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

void enable_ecc(uint32_t mask)
{
	SYSCON->ECC_ENABLE_CTRL = mask;
}
