/*
 * Copyright (c) 2023 Jerzy Kasenberg.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <soc.h>

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	int ret = 0;
	uint32_t reason = CRG_TOP->RESET_STAT_REG;
	uint32_t flags = 0;

	/*
	 * When POR is detected other bits are not valid.
	 */
	if (reason & CRG_TOP_RESET_STAT_REG_PORESET_STAT_Msk) {
		flags = RESET_POR;
	} else {
		if (reason & CRG_TOP_RESET_STAT_REG_HWRESET_STAT_Msk) {
			flags |= RESET_PIN;
		}
		if (reason & CRG_TOP_RESET_STAT_REG_SWRESET_STAT_Msk) {
			flags |= RESET_SOFTWARE;
		}
		if (reason & CRG_TOP_RESET_STAT_REG_WDOGRESET_STAT_Msk) {
			flags |= RESET_WATCHDOG;
		}
		if (reason & CRG_TOP_RESET_STAT_REG_CMAC_WDOGRESET_STAT_Msk) {
			flags |= RESET_WATCHDOG;
		}
		if (reason & CRG_TOP_RESET_STAT_REG_SWD_HWRESET_STAT_Msk) {
			flags |= RESET_DEBUG;
		}
	}

	*cause = flags;

	return ret;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	int ret = 0;

	CRG_TOP->RESET_STAT_REG = 0;

	return ret;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_PIN
		| RESET_SOFTWARE
		| RESET_POR
		| RESET_WATCHDOG
		| RESET_DEBUG);

	return 0;
}
