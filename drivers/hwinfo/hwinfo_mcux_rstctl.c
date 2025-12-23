/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>
#include <fsl_reset.h>

LOG_MODULE_REGISTER(hwinfo_rstctl, CONFIG_HWINFO_LOG_LEVEL);

/* The reset source bits are defined in the rstctl_reset_source_t enum. */
#define MCUX_RESET_POR_FLAG      RSTCTL3_SYSRSTSTAT_VDD_POR_MASK
#define MCUX_RESET_PIN_FLAG      RSTCTL3_SYSRSTSTAT_RESETN_RESET_MASK
#define MCUX_RESET_DEBUG_FLAG    RSTCTL3_SYSRSTSTAT_ISP_AP_RESET_MASK
#define MCUX_RESET_SOFTWARE_FLAG RSTCTL3_SYSRSTSTAT_ITRC_SW_RESET_MASK
#define MCUX_RESET_CPU_FLAG                                                         \
	(RSTCTL3_SYSRSTSTAT_CPU0_RESET_MASK | RSTCTL3_SYSRSTSTAT_CPU1_RESET_MASK)
#define MCUX_RESET_WATCHDOG_FLAG                                                    \
	(RSTCTL3_SYSRSTSTAT_WWDT0_RESET_MASK | RSTCTL3_SYSRSTSTAT_WWDT1_RESET_MASK |    \
	 RSTCTL3_SYSRSTSTAT_WWDT2_RESET_MASK | RSTCTL3_SYSRSTSTAT_WWDT3_RESET_MASK |    \
	 RSTCTL3_SYSRSTSTAT_CDOG0_RESET_MASK | RSTCTL3_SYSRSTSTAT_CDOG1_RESET_MASK |    \
	 RSTCTL3_SYSRSTSTAT_CDOG2_RESET_MASK | RSTCTL3_SYSRSTSTAT_CDOG3_RESET_MASK |    \
	 RSTCTL3_SYSRSTSTAT_CDOG4_RESET_MASK)

/**
 * @brief Translate from RSTCTL reset source mask to Zephyr hwinfo sources mask.
 *
 * Translate bitmask from MCUX RSTCTL reset source bitmask to Zephyr
 * hwinfo reset source bitmask.
 *
 * @param NXP MCUX RSTCTL reset source mask.
 * @retval Zephyr hwinfo reset source mask.
 */
static uint32_t hwinfo_mcux_rstctl_xlate_reset_sources(uint32_t sources)
{
	uint32_t mask = 0;

	if (sources & MCUX_RESET_POR_FLAG) {
		mask |= RESET_POR;
	}

	if (sources & MCUX_RESET_PIN_FLAG) {
		mask |= RESET_PIN;
	}

	if (sources & MCUX_RESET_DEBUG_FLAG) {
		mask |= RESET_DEBUG;
	}

	if (sources & MCUX_RESET_SOFTWARE_FLAG) {
		mask |= RESET_SOFTWARE;
	}

	if (sources & MCUX_RESET_CPU_FLAG) {
		mask |= RESET_SOFTWARE;
	}

	if (sources & MCUX_RESET_WATCHDOG_FLAG) {
		mask |= RESET_WATCHDOG;
	}

	return mask;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	const uint32_t sources = RESET_GetPreviousResetSources();

	*cause = hwinfo_mcux_rstctl_xlate_reset_sources(sources);

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	const uint32_t sources = RESET_GetPreviousResetSources();

	RESET_ClearResetSources(sources);

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_WATCHDOG | RESET_PIN | RESET_POR | RESET_USER |
			RESET_SOFTWARE | RESET_DEBUG);

	return 0;
}
