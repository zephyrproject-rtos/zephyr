/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Marcelo Roberto Jimenez
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <soc.h>
#include <xmc_scu.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>

/* g_chipid is filled in soc_reset_hook(), before the RAM is initialized. */
ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	length = MIN(length, sizeof(g_chipid));
	memcpy(buffer, g_chipid, length);

	return length;
}

/*
 * A power-up sets PORST, SWD and PV; a pulse on the PORST pin sets only PORST.
 * The bits accumulate until they are cleared, and a pulse on PORST keeps SWD
 * and PV, so the cause must be cleared after reading it.
 */
int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t reason = XMC_SCU_RESET_GetDeviceResetReason();
	uint32_t flags = 0;

	if (reason & XMC_SCU_RESET_REASON_PORST) {
		flags |= RESET_PIN;
	}
	if (reason & XMC_SCU_RESET_REASON_SWD) {
		flags |= RESET_BROWNOUT;
	}
	if (reason & XMC_SCU_RESET_REASON_PV) {
		flags |= RESET_POR;
	}
	if (reason & XMC_SCU_RESET_REASON_SW) {
		flags |= RESET_SOFTWARE;
	}
	if (reason & XMC_SCU_RESET_REASON_LOCKUP) {
		flags |= RESET_CPU_LOCKUP;
	}
	if (reason & XMC_SCU_RESET_REASON_WATCHDOG) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & XMC_SCU_RESET_REASON_PARITY_ERROR) {
		flags |= RESET_PARITY;
	}

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	XMC_SCU_RESET_ClearDeviceResetReason();

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_PIN | RESET_BROWNOUT | RESET_POR | RESET_SOFTWARE | RESET_CPU_LOCKUP |
		     RESET_WATCHDOG | RESET_PARITY;

	return 0;
}
