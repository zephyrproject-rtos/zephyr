/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_pmc_hwinfo

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hwinfo_pmc, CONFIG_HWINFO_LOG_LEVEL);

/* PMC peripheral base address, taken from devicetree. */
#define MCUX_PMC ((PMC_Type *)DT_INST_REG_ADDR(0))

/*
 * On LPC55xx the last reset cause is latched by hardware into the always-on
 * PMC->AOREG1 register. The reset-cause bits are only cleared by a Power-On or
 * Brown-Out reset, or by software writing the register.
 *
 * Watchdog reset sources: the windowed watchdog (WDTRESET) plus the optional
 * Code Watchdog (CDOGRESET), which only exists on some LPC55xx parts.
 */
#ifdef PMC_AOREG1_CDOGRESET_MASK
#define MCUX_PMC_WATCHDOG_MASK (PMC_AOREG1_WDTRESET_MASK | PMC_AOREG1_CDOGRESET_MASK)
#else
#define MCUX_PMC_WATCHDOG_MASK (PMC_AOREG1_WDTRESET_MASK)
#endif

#define MCUX_PMC_RESET_CAUSE_MASK                                                          \
	(PMC_AOREG1_POR_MASK | PMC_AOREG1_PADRESET_MASK | PMC_AOREG1_BODRESET_MASK |        \
	 PMC_AOREG1_SYSTEMRESET_MASK | PMC_AOREG1_SWRRESET_MASK | MCUX_PMC_WATCHDOG_MASK |  \
	 PMC_AOREG1_DPDRESET_WAKEUPIO_MASK | PMC_AOREG1_DPDRESET_RTC_MASK |                 \
	 PMC_AOREG1_DPDRESET_OSTIMER_MASK)

/**
 * @brief Translate from PMC AOREG1 reset source mask to Zephyr hwinfo mask.
 *
 * @param sources NXP PMC AOREG1 reset source mask.
 * @retval Zephyr hwinfo reset source mask.
 */
static uint32_t hwinfo_mcux_pmc_xlate_reset_sources(uint32_t sources)
{
	uint32_t mask = 0;

	if (sources & PMC_AOREG1_POR_MASK) {
		mask |= RESET_POR;
	}

	if (sources & PMC_AOREG1_PADRESET_MASK) {
		mask |= RESET_PIN;
	}

	if (sources & PMC_AOREG1_BODRESET_MASK) {
		mask |= RESET_BROWNOUT;
	}

	if (sources & (PMC_AOREG1_SYSTEMRESET_MASK | PMC_AOREG1_SWRRESET_MASK)) {
		mask |= RESET_SOFTWARE;
	}

	if (sources & MCUX_PMC_WATCHDOG_MASK) {
		mask |= RESET_WATCHDOG;
	}

	if (sources & (PMC_AOREG1_DPDRESET_WAKEUPIO_MASK | PMC_AOREG1_DPDRESET_RTC_MASK |
		       PMC_AOREG1_DPDRESET_OSTIMER_MASK)) {
		mask |= RESET_LOW_POWER_WAKE;
	}

	return mask;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t sources = MCUX_PMC->AOREG1 & MCUX_PMC_RESET_CAUSE_MASK;

	*cause = hwinfo_mcux_pmc_xlate_reset_sources(sources);

	LOG_DBG("sources = 0x%08x, cause = 0x%08x", sources, *cause);

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	MCUX_PMC->AOREG1 &= ~(uint32_t)MCUX_PMC_RESET_CAUSE_MASK;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = hwinfo_mcux_pmc_xlate_reset_sources(MCUX_PMC_RESET_CAUSE_MASK);

	return 0;
}
