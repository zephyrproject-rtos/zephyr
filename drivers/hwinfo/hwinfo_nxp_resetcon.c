/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_resetcon_hwinfo

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_resetcon.h>

LOG_MODULE_REGISTER(hwinfo_nxp_resetcon, CONFIG_HWINFO_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Exactly one nxp,resetcon-hwinfo node must be enabled");

#define RESETCON_BASE ((RESETCON_Type *)DT_INST_REG_ADDR(0))

struct resetcon_cause_map {
	uint32_t sources; /* _resetcon_reset_source_flags bits */
	uint32_t cause;   /* RESET_* flags */
};

/* One entry per RESET_* cause, ordered by its first RSTSTAT_STICKY source. */
static const struct resetcon_cause_map resetcon_cause_map[] = {
	{
		.sources = kRESETCON_SrcPorVbat | kRESETCON_SrcPmuPor | kRESETCON_SrcPinPor,
		.cause = RESET_POR,
	},
	{
		/* Set by the PMU voltage monitors. */
		.sources = kRESETCON_SrcPmuCold | kRESETCON_SrcPmuWarm,
		.cause = RESET_BROWNOUT,
	},
	{
		/* Wakeup from Deep Power Down or Power Down. */
		.sources = kRESETCON_SrcWuVbat | kRESETCON_SrcWuPd,
		.cause = RESET_LOW_POWER_WAKE,
	},
	{
		/* CSS security reset, security violation, or TDET tampering. */
		.sources = kRESETCON_SrcSecCold | kRESETCON_SrcSecvio | kRESETCON_SrcTamper,
		.cause = RESET_SECURITY,
	},
	{
		/*
		 * A warm reset escalated to cold is reported as hardware. RESETCON
		 * also reports on-chip module errors during a reset sequence here.
		 */
		.sources = kRESETCON_SrcWarmEsca | kRESETCON_SrcRstErr,
		.cause = RESET_HARDWARE,
	},
	{
		.sources = kRESETCON_SrcPinReset,
		.cause = RESET_PIN,
	},
	{
		.sources = kRESETCON_SrcResetSw | kRESETCON_SrcCpu0Sw,
		.cause = RESET_SOFTWARE,
	},
	{
		.sources = kRESETCON_SrcDap | kRESETCON_SrcJtag,
		.cause = RESET_DEBUG,
	},
	{
		.sources = kRESETCON_SrcCpu0Err,
		.cause = RESET_CPU_LOCKUP,
	},
	{
		.sources = kRESETCON_SrcSwt0 | kRESETCON_SrcSwt1,
		.cause = RESET_WATCHDOG,
	},
};

static uint32_t hwinfo_nxp_resetcon_xlate(uint32_t sources)
{
	uint32_t cause = 0U;

	ARRAY_FOR_EACH_PTR(resetcon_cause_map, entry) {
		if ((sources & entry->source) != 0U) {
			cause |= entry->cause;
		}
	}

	return cause;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	const uint32_t sources = RESETCON_GetStickyResetStatus(RESETCON_BASE);

	*cause = hwinfo_nxp_resetcon_xlate(sources);

	LOG_DBG("sources = 0x%08x, cause = 0x%08x", sources, *cause);

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	RESETCON_ClearStickyResetStatus(RESETCON_BASE, kRESETCON_SrcAll);

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	uint32_t cause = 0U;

	ARRAY_FOR_EACH_PTR(resetcon_cause_map, entry) {
		cause |= entry->cause;
	}

	*supported = cause;

	return 0;
}
