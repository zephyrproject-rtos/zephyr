/*
 * Copyright 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_common.h>
#include <fsl_clock.h>

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

void BOARD_EnableXspiCache(CACHE64_CTRL_Type *cache)
{
	cache->CCR |= CACHE64_CTRL_CCR_INVW0_MASK | CACHE64_CTRL_CCR_INVW1_MASK |\
	CACHE64_CTRL_CCR_GO_MASK; /* First, invalidate the entire cache. */
	while ((cache->CCR & CACHE64_CTRL_CCR_GO_MASK) != 0x00U) {
	}
	/* As a precaution clear the bits to avoid inadvertently re-running this command. */
	cache->CCR &= ~(CACHE64_CTRL_CCR_INVW0_MASK | CACHE64_CTRL_CCR_INVW1_MASK);
	/* Now enable the cache. */
	cache->CCR |= CACHE64_CTRL_CCR_ENCACHE_MASK;
}

void BOARD_DisableXspiCache(CACHE64_CTRL_Type *cache)
{
	cache->CCR |= CACHE64_CTRL_CCR_PUSHW0_MASK | CACHE64_CTRL_CCR_PUSHW1_MASK |\
	CACHE64_CTRL_CCR_GO_MASK; /* First, clean XSPI cache. */
	while ((cache->CCR & CACHE64_CTRL_CCR_GO_MASK) != 0x00U) {
	}
	/* As a precaution clear the bits to avoid inadvertently re-running this command. */
	cache->CCR &= ~(CACHE64_CTRL_CCR_PUSHW0_MASK | CACHE64_CTRL_CCR_PUSHW1_MASK);

	/* Now disable XSPI cache. */
	cache->CCR &= ~CACHE64_CTRL_CCR_ENCACHE_MASK;
}

void BOARD_DeinitXspi(XSPI_Type *base, CACHE64_CTRL_Type *cache)
{
	if (base == XSPI0) {
		/* Enable clock again. */
		CLKCTL0->PSCCTL1_SET = CLKCTL0_PSCCTL1_CLR_XSPI0_MASK;
	} else if (base == XSPI1) {
		/* Enable clock again. */
		CLKCTL0->PSCCTL1_SET = CLKCTL0_PSCCTL1_CLR_XSPI1_MASK;
	}

	base->MCR &= ~XSPI_MCR_MDIS_MASK;
	if ((cache->CCR & CACHE64_CTRL_CCR_ENCACHE_MASK) != 0x00U) {
		BOARD_DisableXspiCache(cache);
	}
	/* Wait until XSPI is not busy */
	while ((base->SR & XSPI_SR_BUSY_MASK) != 0U) {
	}
	/* Disable module. */
	base->MCR |= XSPI_MCR_MDIS_MASK;
}

void BOARD_InitXspi(XSPI_Type *base, CACHE64_CTRL_Type *cache)
{
	/* Enable XSPI module */
	base->MCR |= XSPI_MCR_MDIS_MASK;

	base->MCR |= XSPI_MCR_SWRSTSD_MASK | XSPI_MCR_SWRSTHD_MASK;
	for (uint32_t i = 0; i < 6; i++) {
		__NOP();
	}
	base->MCR &= ~(XSPI_MCR_SWRSTSD_MASK | XSPI_MCR_SWRSTHD_MASK);
	base->MCR |= XSPI_MCR_IPS_TG_RST_MASK | XSPI_MCR_MDIS_MASK;
	base->MCR &= ~XSPI_MCR_ISD3FA_MASK;
	base->MCR &= ~XSPI_MCR_MDIS_MASK;
	base->MCR |= XSPI_MCR_MDIS_MASK;
	base->MCR |= XSPI_MCR_ISD3FA_MASK;
	base->MCR &= ~XSPI_MCR_MDIS_MASK;

	base->MCR |= XSPI_MCR_MDIS_MASK;
	base->SMPR = (((base->SMPR) & (~XSPI_SMPR_DLLFSMPFA_MASK)) |
			XSPI_SMPR_DLLFSMPFA(FSL_FEATURE_XSPI_DLL_REF_VALUE_DDR_DELAY_TAP_NUM));
	base->MCR &= ~XSPI_MCR_MDIS_MASK;

	base->DLLCR[0] &= ~(XSPI_DLLCR_SLV_DLL_BYPASS_MASK | XSPI_DLLCR_DLL_CDL8_MASK |\
		XSPI_DLLCR_SLV_DLY_OFFSET_MASK | XSPI_DLLCR_SLV_FINE_OFFSET_MASK |\
		XSPI_DLLCR_DLLRES_MASK | XSPI_DLLCR_DLL_REFCNTR_MASK | XSPI_DLLCR_FREQEN_MASK);
	base->DLLCR[0] &= ~(XSPI_DLLCR_SLV_EN_MASK | XSPI_DLLCR_SLAVE_AUTO_UPDT_MASK |\
			XSPI_DLLCR_DLLEN_MASK);
	/* Enable subordinate as auto update mode. */
	base->DLLCR[0] |= XSPI_DLLCR_SLV_EN_MASK | XSPI_DLLCR_SLAVE_AUTO_UPDT_MASK;
	/* program DLL to desired delay. */
	base->DLLCR[0] |= XSPI_DLLCR_DLLRES(FSL_FEATURE_XSPI_DLL_REF_VALUE_AUTOUPDATE_X16_DISABLE_RES)|\
	XSPI_DLLCR_DLL_REFCNTR(FSL_FEATURE_XSPI_DLL_REF_VALUE_AUTOUPDATE_X16_DISABLED_REF_COUNTER) |\
	XSPI_DLLCR_SLV_FINE_OFFSET(0) | XSPI_DLLCR_SLV_DLY_OFFSET(0) | XSPI_DLLCR_FREQEN(1U);
	/* Load above settings into delay chain. */
	base->DLLCR[0] |= XSPI_DLLCR_SLV_UPD_MASK;
	base->DLLCR[0] |= XSPI_DLLCR_DLLEN_MASK;
	base->DLLCR[0] &= ~XSPI_DLLCR_SLV_UPD_MASK;

	while ((base->DLLSR & XSPI_DLLSR_SLVA_LOCK_MASK) == 0UL) {
	}

	if ((cache->CCR & CACHE64_CTRL_CCR_ENCACHE_MASK) == 0x00U) {
		BOARD_EnableXspiCache(cache);
		/* flush pipeline */
		__DSB();
		__ISB();
	}
}

/* BOARD_SetXspiClock run in RAM when XIP. */
void BOARD_SetXspiClock(XSPI_Type *base, uint32_t src, uint32_t divider)
{
	if (base == XSPI0) {
		if ((CLKCTL0->XSPI0FCLKSEL != CLKCTL0_XSPI0FCLKSEL_SEL(src))\
		|| ((CLKCTL0->XSPI0FCLKDIV & CLKCTL0_XSPI0FCLKDIV_DIV_MASK) != (divider - 1))) {
			/* Always deinit XSPI and init XSPI for the flash to make
			 * sure the flash works correctly after the XSPI root clock
			 * changed as the default XSPI configuration may does not
			 * work for the new root clock frequency.
			 */
			BOARD_DeinitXspi(base, CACHE64_CTRL0);

			/* Disable clock before changing clock source */
			CLKCTL0->PSCCTL1_CLR = CLKCTL0_PSCCTL1_CLR_XSPI0_MASK;
			/* Update XSPI clock. */
			CLKCTL0->XSPI0FCLKSEL = CLKCTL0_XSPI0FCLKSEL_SEL(src)\
			| CLKCTL0_XSPI0FCLKSEL_SEL_EN_MASK;
			CLKCTL0->XSPI0FCLKDIV = CLKCTL0_XSPI0FCLKDIV_DIV(divider - 1);
			while ((CLKCTL0->XSPI0FCLKDIV) & CLKCTL0_XSPI0FCLKDIV_REQFLAG_MASK) {
			}
			/* Enable XSPI clock again */
			CLKCTL0->PSCCTL1_SET = CLKCTL0_PSCCTL1_SET_XSPI0_MASK;

			BOARD_InitXspi(base, CACHE64_CTRL0);
		}
	} else if (base == XSPI1) {
		if ((CLKCTL0->XSPI1FCLKSEL != CLKCTL0_XSPI1FCLKSEL_SEL(src)) ||\
			((CLKCTL0->XSPI1FCLKDIV & CLKCTL0_XSPI1FCLKDIV_DIV_MASK) !=\
			(divider - 1))) {
			/* Always deinit XSPI and init XSPI for the flash to make sure the flash
			 * works correctly after the XSPI root clock changed as the default XSPI
			 * configuration may does not work for the new root clock frequency.
			 */
			BOARD_DeinitXspi(base, CACHE64_CTRL1);

			/* Disable clock before changing clock source */
			CLKCTL0->PSCCTL1_CLR = CLKCTL0_PSCCTL1_CLR_XSPI1_MASK;
			/* Update XSPI clock. */
			CLKCTL0->XSPI1FCLKSEL = CLKCTL0_XSPI1FCLKSEL_SEL(src)\
				| CLKCTL0_XSPI1FCLKSEL_SEL_EN_MASK;
			CLKCTL0->XSPI1FCLKDIV = CLKCTL0_XSPI1FCLKDIV_DIV(divider - 1);
			while ((CLKCTL0->XSPI1FCLKDIV) & CLKCTL0_XSPI1FCLKDIV_REQFLAG_MASK) {
			}
			/* Enable XSPI clock again */
			CLKCTL0->PSCCTL1_SET = CLKCTL0_PSCCTL1_SET_XSPI1_MASK;

			BOARD_InitXspi(base, CACHE64_CTRL1);
		}
	}
}

void BOARD_XspiClockSafeConfig(void)
{
	BOARD_SetXspiClock(XSPI0, 0U, 1U);
	BOARD_SetXspiClock(XSPI1, 0U, 1U);
}
