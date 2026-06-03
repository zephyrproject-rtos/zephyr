/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>

#include <string.h>

#include <sl_hal_emu.h>
#include <sl_hal_system.h>

/* The Zephyr API expects hwinfo_get_reset_cause() to return 0 after hwinfo_clear_reset_cause() has
 * been called. This matches the hardware behavior on Series 3, but not the HAL API. The HAL stores
 * the reset cause upon first read, and returns this cached value on subsequent calls to the API
 * to allow multiple subsystems to read the reset cause despite it having been cleared in hardware
 * already. Emulate the hardware behavior while staying compatible with other users of the HAL API
 * by keeping track of whether the reset cause should be considered cleared or not ourselves.
 */
static bool reset_cleared;

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint64_t unique_id = sys_cpu_to_be64(sl_hal_system_get_unique());

	if (length > sizeof(unique_id)) {
		length = sizeof(unique_id);
	}

	memcpy(buffer, &unique_id, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
	uint32_t reset = sl_hal_emu_get_reset_cause();

	if (reset_cleared) {
		*cause = 0;
		return 0;
	}

	if (reset & EMU_RSTCAUSE_POR) {
		flags |= RESET_POR;
	}

	if (reset & EMU_RSTCAUSE_PIN) {
		flags |= RESET_PIN;
	}

	if (reset & EMU_RSTCAUSE_EM4) {
		flags |= RESET_LOW_POWER_WAKE;
	}

	if (reset & (EMU_RSTCAUSE_WDOG0 | EMU_RSTCAUSE_WDOG1)) {
		flags |= RESET_WATCHDOG;
	}

	if (reset & EMU_RSTCAUSE_LOCKUP) {
		flags |= RESET_CPU_LOCKUP;
	}

	if (reset & EMU_RSTCAUSE_SYSREQ) {
		flags |= RESET_SOFTWARE;
	}

	if (reset & (EMU_RSTCAUSE_DVDDBOD | EMU_RSTCAUSE_DVDDLEBOD | EMU_RSTCAUSE_DECBOD |
		     EMU_RSTCAUSE_AVDDBOD | EMU_RSTCAUSE_IOVDD0BOD | EMU_RSTCAUSE_IOVDD1BOD |
		     EMU_RSTCAUSE_FLBOD)) {
		flags |= RESET_BROWNOUT;
	}

	if (reset & EMU_RSTCAUSE_SETAMPER) {
		flags |= RESET_SECURITY;
	}

	*cause = flags;
	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	sl_hal_emu_clear_reset_cause();
	reset_cleared = true;
	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_PIN | RESET_SOFTWARE | RESET_BROWNOUT | RESET_POR | RESET_WATCHDOG |
		     RESET_SECURITY | RESET_LOW_POWER_WAKE | RESET_CPU_LOCKUP;
	return 0;
}
