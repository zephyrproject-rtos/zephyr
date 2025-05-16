/*
 * Copyright (c) 2025 Baylibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <string.h>

#include <driverlib/pmctl.h>

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_PIN
		      | RESET_SOFTWARE
		      | RESET_BROWNOUT
		      | RESET_POR
		      | RESET_WATCHDOG
		      | RESET_DEBUG
		      | RESET_CPU_LOCKUP
		      | RESET_CLOCK
		      | RESET_TEMPERATURE);

	return 0;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t rststa = PMCTLGetResetReason();

	switch (rststa) {
	case PMCTL_RESET_PIN:
		*cause = RESET_PIN;
		break;
	case PMCTL_RESET_SYSTEM:
		*cause = RESET_SOFTWARE;
		break;
	case PMCTL_RESET_VDDR:
	case PMCTL_RESET_VDDS:
		*cause = RESET_BROWNOUT;
		break;
	case PMCTL_RESET_POR:
		*cause = RESET_POR;
		break;
	case PMCTL_RESET_WATCHDOG:
		*cause = RESET_WATCHDOG;
		break;
	case PMCTL_RESET_SWD:
	case PMCTL_RESET_SHUTDOWN_SWD:
		*cause = RESET_DEBUG;
		break;
	case PMCTL_RESET_LOCKUP:
		*cause = RESET_CPU_LOCKUP;
		break;
	case PMCTL_RESET_LFXT:
		*cause = RESET_CLOCK;
		break;
	case PMCTL_RESET_TSD:
		*cause = RESET_TEMPERATURE;
		break;
	default:
		*cause = 0;
		break;
	}

	return 0;
}
