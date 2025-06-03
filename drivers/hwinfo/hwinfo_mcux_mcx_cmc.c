/*
 * Copyright (c) 2025 Adib Taraben
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>
#include <fsl_cmc.h>

LOG_MODULE_REGISTER(hwinfo_cmc, CONFIG_HWINFO_LOG_LEVEL);

#ifndef CMC0
#define CMC0 CMC
#endif

#ifdef CMC_SRS_VBAT_MASK
#define CMC_RESET_MASK_POR (CMC_SRS_POR_MASK | CMC_SRS_VBAT_MASK)
#else
#define CMC_RESET_MASK_POR CMC_SRS_POR_MASK
#endif

#ifdef CMC_SRS_WWDT1_MASK
#define CMC_RESET_MASK_WATCHDOG (CMC_SRS_WWDT0_MASK | CMC_SRS_WWDT1_MASK)
#else
#define CMC_RESET_MASK_WATCHDOG CMC_SRS_WWDT0_MASK
#endif

#ifdef CMC_SRS_CDOG1_MASK
#define CMC_RESET_MASK_CDOG (CMC_SRS_CDOG0_MASK | CMC_SRS_CDOG1_MASK)
#else
#define CMC_RESET_MASK_CDOG CMC_SRS_CDOG0_MASK
#endif

/**
 * @brief Translate from CMC reset source mask to Zephyr hwinfo sources mask.
 *
 * Translate bitmask from MCUX CMC reset source bitmask to Zephyr
 * hwinfo reset source bitmask.
 *
 * @param NXP MCUX CMC reset source mask.
 * @retval Zephyr hwinfo reset source mask.
 */
static uint32_t hwinfo_mcux_cmc_xlate_reset_sources(uint32_t sources)
{
	uint32_t mask = 0;

	/* order of tests below according to SRS register definitions */
	if (sources & CMC_SRS_WAKEUP_MASK) {
		mask |= RESET_LOW_POWER_WAKE;
	}

	if (sources & CMC_RESET_MASK_POR) {
		mask |= RESET_POR;
	}

	if (sources & CMC_SRS_VD_MASK) {
		mask |= RESET_BROWNOUT;
	}

	if (sources & CMC_SRS_PIN_MASK) {
		mask |= RESET_PIN;
	}

	if (sources & (CMC_SRS_JTAG_MASK | CMC_SRS_DAP_MASK)) {
		mask |= RESET_DEBUG;
	}

	if (sources & CMC_SRS_SCG_MASK) {
		mask |= RESET_CLOCK;
	}

	if (sources & CMC_RESET_MASK_WATCHDOG) {
		mask |= RESET_WATCHDOG;
	}

	if (sources & CMC_SRS_SW_MASK) {
		mask |= RESET_SOFTWARE;
	}

	if (sources & CMC_SRS_LOCKUP_MASK) {
		mask |= RESET_CPU_LOCKUP;
	}

	if (sources & CMC_RESET_MASK_CDOG) {
		mask |= RESET_WATCHDOG;
	}

#ifdef CMC_SRS_SECVIO_MASK
	if (sources & CMC_SRS_SECVIO_MASK) {
		mask |= RESET_SECURITY;
	}
#endif

	return mask;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	const uint32_t sources = CMC_GetStickySystemResetStatus(CMC0);

	*cause = hwinfo_mcux_cmc_xlate_reset_sources(sources);

	LOG_DBG("sources = 0x%08x, cause = 0x%08x", sources, *cause);

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	const uint32_t sources = CMC_GetStickySystemResetStatus(CMC0);

	CMC_ClearStickySystemResetStatus(CMC0, sources);

	LOG_DBG("sources = 0x%08x", sources);

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = hwinfo_mcux_cmc_xlate_reset_sources(UINT32_MAX);

	LOG_DBG("supported = 0x%08x", *supported);

	return 0;
}
