/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>

#include <fsl_ocotp.h>
#include <fsl_power.h>

/* Because of the ROM clearing the reset register and using scratch register
 * which cannot be cleared, we have to "fake" this to meet the hwinfo api.
 * Technically all the reset causes are already cleared by the ROM, but we will
 * still clear them ourselves on the first call to clear them by user.
 */
static bool reset_cleared;

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint32_t id_length = length;

	if (OCOTP_ReadUniqueID(buffer, &id_length)) {
		return -EINVAL;
	}

	return (ssize_t)id_length;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (
		RESET_SOFTWARE		|
		RESET_CPU_LOCKUP	|
		RESET_WATCHDOG		|
		RESET_SECURITY		|
		RESET_DEBUG		|
		RESET_HARDWARE
	);

	return 0;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	if (reset_cleared) {
		*cause = 0;
		return 0;
	}

	uint32_t reset_cause = POWER_GetResetCause();

	*cause = 0;

	if (reset_cause & kPOWER_ResetCauseSysResetReq) {
		*cause |= RESET_SOFTWARE;
	}
	if (reset_cause & kPOWER_ResetCauseLockup) {
		*cause |= RESET_CPU_LOCKUP;
	}
	if (reset_cause & kPOWER_ResetCauseWdt) {
		*cause |= RESET_WATCHDOG;
	}
	if (reset_cause & kPOWER_ResetCauseApResetReq) {
		*cause |= RESET_DEBUG;
	}
	if (reset_cause & (kPOWER_ResetCauseCodeWdt | kPOWER_ResetCauseItrc)) {
		*cause |= RESET_SECURITY;
	}
	if (reset_cause & kPOWER_ResetCauseResetB) {
		*cause |= RESET_HARDWARE;
	}

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	POWER_ClearResetCause(kPOWER_ResetCauseAll);

	reset_cleared = true;

	return 0;
}
