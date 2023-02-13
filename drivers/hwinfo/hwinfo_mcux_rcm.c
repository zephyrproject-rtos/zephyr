/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>
#include <fsl_rcm.h>

LOG_MODULE_REGISTER(hwinfo_rcm, CONFIG_HWINFO_LOG_LEVEL);

/**
 * @brief Translate from RCM reset source mask to Zephyr hwinfo sources mask.
 *
 * Translate bitmask from MCUX RCM reset source bitmask to Zephyr
 * hwinfo reset source bitmask.
 *
 * @param NXP MCUX RCM reset source mask.
 * @retval Zephyr hwinfo reset source mask.
 */
static uint32_t hwinfo_mcux_rcm_xlate_reset_sources(uint32_t sources)
{
	uint32_t mask = 0;

#if (defined(FSL_FEATURE_RCM_HAS_WAKEUP) && FSL_FEATURE_RCM_HAS_WAKEUP)
	if (sources & kRCM_SourceWakeup) {
		mask |= RESET_LOW_POWER_WAKE;
	}
#endif /* (defined(FSL_FEATURE_RCM_HAS_WAKEUP) && FSL_FEATURE_RCM_HAS_WAKEUP) */

	if (sources & kRCM_SourceLvd) {
		mask |= RESET_BROWNOUT;
	}

#if (defined(FSL_FEATURE_RCM_HAS_LOC) && FSL_FEATURE_RCM_HAS_LOC)
	if (sources & kRCM_SourceLoc) {
		mask |= RESET_CLOCK;
	}
#endif /* (defined(FSL_FEATURE_RCM_HAS_LOC) && FSL_FEATURE_RCM_HAS_LOC) */

#if (defined(FSL_FEATURE_RCM_HAS_LOL) && FSL_FEATURE_RCM_HAS_LOL)
	if (sources & kRCM_SourceLol) {
		mask |= RESET_PLL;
	}
#endif /*  (defined(FSL_FEATURE_RCM_HAS_LOL) && FSL_FEATURE_RCM_HAS_LOL) */

	if (sources & kRCM_SourceWdog) {
		mask |= RESET_WATCHDOG;
	}

	if (sources & kRCM_SourcePin) {
		mask |= RESET_PIN;
	}

	if (sources & kRCM_SourcePor) {
		mask |= RESET_POR;
	}

#if (defined(FSL_FEATURE_RCM_HAS_JTAG) && FSL_FEATURE_RCM_HAS_JTAG)
	if (sources & kRCM_SourceJtag) {
		mask |= RESET_DEBUG;
	}
#endif /* (defined(FSL_FEATURE_RCM_HAS_JTAG) && FSL_FEATURE_RCM_HAS_JTAG) */

	if (sources & kRCM_SourceLockup) {
		mask |= RESET_CPU_LOCKUP;
	}

	if (sources & kRCM_SourceSw) {
		mask |= RESET_SOFTWARE;
	}

#if (defined(FSL_FEATURE_RCM_HAS_MDM_AP) && FSL_FEATURE_RCM_HAS_MDM_AP)
	if (sources & kRCM_SourceMdmap) {
		mask |= RESET_DEBUG;
	}
#endif /* (defined(FSL_FEATURE_RCM_HAS_MDM_AP) && FSL_FEATURE_RCM_HAS_MDM_AP) */

#if (defined(FSL_FEATURE_RCM_HAS_EZPORT) && FSL_FEATURE_RCM_HAS_EZPORT)
	if (sources & kRCM_SourceEzpt) {
		mask |= RESET_DEBUG;
	}
#endif /*  (defined(FSL_FEATURE_RCM_HAS_EZPORT) && FSL_FEATURE_RCM_HAS_EZPORT) */

	return mask;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t sources;

#if (defined(FSL_FEATURE_RCM_HAS_SSRS) && FSL_FEATURE_RCM_HAS_SSRS)
	sources = RCM_GetStickyResetSources(RCM) & kRCM_SourceAll;
#else /* (defined(FSL_FEATURE_RCM_HAS_SSRS) && FSL_FEATURE_RCM_HAS_SSRS) */
	sources = RCM_GetPreviousResetSources(RCM) & kRCM_SourceAll;
#endif /* !(defined(FSL_FEATURE_RCM_HAS_PARAM) && FSL_FEATURE_RCM_HAS_PARAM) */

	*cause = hwinfo_mcux_rcm_xlate_reset_sources(sources);

	LOG_DBG("sources = 0x%08x, cause = 0x%08x", sources, *cause);

	return 0;
}

#if (defined(FSL_FEATURE_RCM_HAS_SSRS) && FSL_FEATURE_RCM_HAS_SSRS)
int z_impl_hwinfo_clear_reset_cause(void)
{
	uint32_t sources;

	sources = RCM_GetStickyResetSources(RCM) & kRCM_SourceAll;
	RCM_ClearStickyResetSources(RCM, sources);

	LOG_DBG("sources = 0x%08x", sources);

	return 0;
}
#endif /* (defined(FSL_FEATURE_RCM_HAS_SSRS) && FSL_FEATURE_RCM_HAS_SSRS) */

#if (defined(FSL_FEATURE_RCM_HAS_PARAM) && FSL_FEATURE_RCM_HAS_PARAM)
int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	uint32_t sources;

	sources = RCM_GetResetSourceImplementedStatus(RCM);
	*supported = hwinfo_mcux_rcm_xlate_reset_sources(sources);

	LOG_DBG("sources = 0x%08x, supported = 0x%08x", sources, *supported);

	return 0;
}
#endif /* (defined(FSL_FEATURE_RCM_HAS_PARAM) && FSL_FEATURE_RCM_HAS_PARAM) */
