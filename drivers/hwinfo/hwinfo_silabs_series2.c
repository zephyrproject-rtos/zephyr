/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include <string.h>

#include <em_system.h>
#include <em_rmu.h>

/* Ensure that all possible reset causes have a definition */
#ifndef EMU_RSTCAUSE_BOOSTON
#define EMU_RSTCAUSE_BOOSTON 0
#endif
#ifndef EMU_RSTCAUSE_WDOG1
#define EMU_RSTCAUSE_WDOG1 0
#endif
#ifndef EMU_RSTCAUSE_IOVDD1BOD
#define EMU_RSTCAUSE_IOVDD1BOD 0
#endif
#ifndef EMU_RSTCAUSE_IOVDD2BOD
#define EMU_RSTCAUSE_IOVDD2BOD 0
#endif
#ifndef EMU_RSTCAUSE_SETAMPER
#define EMU_RSTCAUSE_SETAMPER 0
#endif
#ifndef EMU_RSTCAUSE_SESYSREQ
#define EMU_RSTCAUSE_SESYSREQ 0
#endif
#ifndef EMU_RSTCAUSE_SELOCKUP
#define EMU_RSTCAUSE_SELOCKUP 0
#endif
#ifndef EMU_RSTCAUSE_DCI
#define EMU_RSTCAUSE_DCI 0
#endif

/* The Zephyr API expects hwinfo_get_reset_cause() to return 0 after hwinfo_clear_reset_cause() has
 * been called. This matches the hardware behavior on Series 2, but not the HAL API. The HAL stores
 * the reset cause upon first read, and returns this cached value on subsequent calls to the API
 * to allow multiple subsystems to read the reset cause despite it having been cleared in hardware
 * already. Emulate the hardware behavior while staying compatible with other users of the HAL API
 * by keeping track of whether the reset cause should be considered cleared or not ourselves.
 */
static bool reset_cleared;

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint64_t unique_id = sys_cpu_to_be64(SYSTEM_GetUnique());

	if (length > sizeof(unique_id)) {
		length = sizeof(unique_id);
	}

	memcpy(buffer, &unique_id, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
	uint32_t rmu = RMU_ResetCauseGet();

	if (reset_cleared) {
		*cause = 0;
		return 0;
	}

	if (rmu & EMU_RSTCAUSE_POR) {
		flags |= RESET_POR;
	}

	if (rmu & EMU_RSTCAUSE_PIN) {
		flags |= RESET_PIN;
	}

	if (rmu & (EMU_RSTCAUSE_EM4 | EMU_RSTCAUSE_BOOSTON)) {
		flags |= RESET_LOW_POWER_WAKE;
	}

	if (rmu & (EMU_RSTCAUSE_WDOG0 | EMU_RSTCAUSE_WDOG1)) {
		flags |= RESET_WATCHDOG;
	}

	if (rmu & EMU_RSTCAUSE_LOCKUP) {
		flags |= RESET_CPU_LOCKUP;
	}

	if (rmu & EMU_RSTCAUSE_SYSREQ) {
		flags |= RESET_SOFTWARE;
	}

	if (rmu & (EMU_RSTCAUSE_DVDDBOD | EMU_RSTCAUSE_DVDDLEBOD | EMU_RSTCAUSE_DECBOD |
		   EMU_RSTCAUSE_AVDDBOD | EMU_RSTCAUSE_IOVDD0BOD |
		   EMU_RSTCAUSE_IOVDD1BOD | EMU_RSTCAUSE_IOVDD2BOD)) {
		flags |= RESET_BROWNOUT;
	}

	if (rmu & (EMU_RSTCAUSE_SETAMPER | EMU_RSTCAUSE_SESYSREQ |
		   EMU_RSTCAUSE_SELOCKUP | EMU_RSTCAUSE_DCI)) {
		flags |= RESET_SECURITY;
	}

	*cause = flags;
	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	RMU_ResetCauseClear();
	reset_cleared = true;
	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_PIN | RESET_SOFTWARE | RESET_BROWNOUT | RESET_POR | RESET_WATCHDOG |
		     RESET_SECURITY | RESET_LOW_POWER_WAKE | RESET_CPU_LOCKUP;
	return 0;
}
