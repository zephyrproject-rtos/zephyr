/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_pmc_hwinfo

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hwinfo_nxp_pmc, CONFIG_HWINFO_LOG_LEVEL);

/* PMC peripheral base address, taken from devicetree. */
#define NXP_PMC ((PMC_Type *)DT_INST_REG_ADDR(0))

/*
 * On LPC55xx the last reset cause is latched by hardware into the always-on
 * PMC->AOREG1 register. The reset-cause bits are only cleared by a Power-On or
 * Brown-Out reset, or by software writing the register.
 *
 * On parts whose header has no AOREG1 reset-cause fields (e.g. MCXW2xx) the
 * boot ROM copies PMC->RESETCAUSE into AOREG1[15:4] and clears RESETCAUSE,
 * so AOREG1 is read using the RESETCAUSE bit layout shifted by 4.
 *
 * Watchdog reset sources: the windowed watchdog (WDTRESET) plus the optional
 * Code Watchdog (CDOGRESET), which only exists on some parts.
 */
#ifdef PMC_AOREG1_WDTRESET_MASK
#define NXP_PMC_RST(name) PMC_AOREG1_##name##_MASK
#ifdef PMC_AOREG1_CDOGRESET_MASK
#define NXP_PMC_CDOG_MASK PMC_AOREG1_CDOGRESET_MASK
#endif
#else
/* Boot ROM stores PMC->RESETCAUSE in AOREG1[15:4]. */
#define NXP_PMC_AOREG1_RESETCAUSE_SHIFT 4U
#define NXP_PMC_RST(name) (PMC_RESETCAUSE_##name##_MASK << NXP_PMC_AOREG1_RESETCAUSE_SHIFT)
#ifdef PMC_RESETCAUSE_CDOGRESET_MASK
#define NXP_PMC_CDOG_MASK (PMC_RESETCAUSE_CDOGRESET_MASK << NXP_PMC_AOREG1_RESETCAUSE_SHIFT)
#endif
#endif

#ifdef NXP_PMC_CDOG_MASK
#define NXP_PMC_WATCHDOG_MASK (NXP_PMC_RST(WDTRESET) | NXP_PMC_CDOG_MASK)
#else
#define NXP_PMC_WATCHDOG_MASK (NXP_PMC_RST(WDTRESET))
#endif

#define NXP_PMC_DPD_MASK                                                                           \
	(NXP_PMC_RST(DPDRESET_WAKEUPIO) | NXP_PMC_RST(DPDRESET_RTC) | NXP_PMC_RST(DPDRESET_OSTIMER))

#define NXP_PMC_RESET_CAUSE_MASK                                                                   \
	(NXP_PMC_RST(POR) | NXP_PMC_RST(PADRESET) | NXP_PMC_RST(BODRESET) |                        \
	 NXP_PMC_RST(SYSTEMRESET) | NXP_PMC_RST(SWRRESET) | NXP_PMC_WATCHDOG_MASK |                \
	 NXP_PMC_DPD_MASK)

/**
 * @brief Translate from PMC AOREG1 reset source mask to Zephyr hwinfo mask.
 *
 * @param sources NXP PMC AOREG1 reset source mask.
 * @retval Zephyr hwinfo reset source mask.
 */
static uint32_t hwinfo_nxp_pmc_xlate_reset_sources(uint32_t sources)
{
	uint32_t mask = 0;

	if (sources & NXP_PMC_RST(POR)) {
		mask |= RESET_POR;
	}

	if (sources & NXP_PMC_RST(PADRESET)) {
		mask |= RESET_PIN;
	}

	if (sources & NXP_PMC_RST(BODRESET)) {
		mask |= RESET_BROWNOUT;
	}

	if (sources & (NXP_PMC_RST(SYSTEMRESET) | NXP_PMC_RST(SWRRESET))) {
		mask |= RESET_SOFTWARE;
	}

	if (sources & NXP_PMC_WATCHDOG_MASK) {
		mask |= RESET_WATCHDOG;
	}

	if (sources & NXP_PMC_DPD_MASK) {
		mask |= RESET_LOW_POWER_WAKE;
	}

	return mask;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t sources = NXP_PMC->AOREG1 & NXP_PMC_RESET_CAUSE_MASK;

	*cause = hwinfo_nxp_pmc_xlate_reset_sources(sources);

	LOG_DBG("sources = 0x%08x, cause = 0x%08x", sources, *cause);

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	NXP_PMC->AOREG1 &= ~(uint32_t)NXP_PMC_RESET_CAUSE_MASK;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = hwinfo_nxp_pmc_xlate_reset_sources(NXP_PMC_RESET_CAUSE_MASK);

	return 0;
}
