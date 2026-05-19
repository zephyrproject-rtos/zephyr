/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>

#include <cy_syslib.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint64_t uid = sys_cpu_to_be64(Cy_SysLib_GetUniqueId());

	length = MIN(length, sizeof(uid));
	memcpy(buffer, &uid, length);

	return length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
	uint32_t reason = Cy_SysLib_GetResetReason();

	if (reason & (CY_SYSLIB_RESET_HWWDT | CY_SYSLIB_RESET_SWWDT0 | CY_SYSLIB_RESET_SWWDT1 |
		      CY_SYSLIB_RESET_SWWDT2 | CY_SYSLIB_RESET_SWWDT3)) {
		flags |= RESET_WATCHDOG;
	}

#if defined(CY_SYSLIB_RESET_SOFT0)
	if (reason & (CY_SYSLIB_RESET_SOFT0 | CY_SYSLIB_RESET_SOFT1 | CY_SYSLIB_RESET_SOFT2)) {
		flags |= RESET_SOFTWARE;
	}
#else
	if (reason & CY_SYSLIB_RESET_SOFT) {
		flags |= RESET_SOFTWARE;
	}
#endif

#ifdef CY_SYSLIB_RESET_PORVDDD
	if (reason & CY_SYSLIB_RESET_PORVDDD) {
		flags |= RESET_POR;
	}
#endif

#if defined(CY_SYSLIB_RESET_BODVDDD) || defined(CY_SYSLIB_RESET_BODVCCD) ||                        \
	defined(CY_SYSLIB_RESET_BODVBAT)
	if (reason & (0U
#ifdef CY_SYSLIB_RESET_BODVDDD
		      | CY_SYSLIB_RESET_BODVDDD
#endif
#ifdef CY_SYSLIB_RESET_BODVCCD
		      | CY_SYSLIB_RESET_BODVCCD
#endif
#ifdef CY_SYSLIB_RESET_BODVBAT
		      | CY_SYSLIB_RESET_BODVBAT
#endif
		      )) {
		flags |= RESET_BROWNOUT;
	}
#endif

#if defined(CY_SYSLIB_RESET_XRES) || defined(CY_SYSLIB_RESET_PXRES) ||                             \
	defined(CY_SYSLIB_RESET_STRUCT_XRES)
	if (reason & (0U
#ifdef CY_SYSLIB_RESET_XRES
		      | CY_SYSLIB_RESET_XRES
#endif
#ifdef CY_SYSLIB_RESET_PXRES
		      | CY_SYSLIB_RESET_PXRES
#endif
#ifdef CY_SYSLIB_RESET_STRUCT_XRES
		      | CY_SYSLIB_RESET_STRUCT_XRES
#endif
		      )) {
		flags |= RESET_PIN;
	}
#endif

#ifdef CY_SYSLIB_RESET_TC_DBGRESET
	if (reason & CY_SYSLIB_RESET_TC_DBGRESET) {
		flags |= RESET_DEBUG;
	}
#endif

	if (reason & (0U
#ifdef CY_SYSLIB_RESET_HIB_WAKEUP
		      | CY_SYSLIB_RESET_HIB_WAKEUP
#endif
#ifdef CY_SYSLIB_RESET_DS_OFF_WAKEUP
		      | CY_SYSLIB_RESET_DS_OFF_WAKEUP
#endif
		      )) {
		flags |= RESET_LOW_POWER_WAKE;
	}

	if (reason & (CY_SYSLIB_RESET_ACT_FAULT | CY_SYSLIB_RESET_DPSLP_FAULT)) {
		flags |= RESET_HARDWARE;
	}

	if (reason & (CY_SYSLIB_RESET_CSV_LOSS_WAKEUP | CY_SYSLIB_RESET_CSV_ERROR_WAKEUP)) {
		flags |= RESET_CLOCK;
	}

#ifdef CY_SYSLIB_RESET_OVDVCCD
	if (reason & CY_SYSLIB_RESET_OVDVCCD) {
		flags |= RESET_BROWNOUT;
	}
#endif

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	Cy_SysLib_ClearResetReason();

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_WATCHDOG | RESET_SOFTWARE | RESET_HARDWARE | RESET_CLOCK |
		      RESET_LOW_POWER_WAKE |
#ifdef CY_SYSLIB_RESET_PORVDDD
		      RESET_POR |
#endif
#if defined(CY_SYSLIB_RESET_BODVDDD) || defined(CY_SYSLIB_RESET_BODVCCD) ||                        \
	defined(CY_SYSLIB_RESET_BODVBAT) || defined(CY_SYSLIB_RESET_OVDVCCD)
		      RESET_BROWNOUT |
#endif
#if defined(CY_SYSLIB_RESET_XRES) || defined(CY_SYSLIB_RESET_PXRES) ||                             \
	defined(CY_SYSLIB_RESET_STRUCT_XRES)
		      RESET_PIN |
#endif
#ifdef CY_SYSLIB_RESET_TC_DBGRESET
		      RESET_DEBUG |
#endif
		      0U);

	return 0;
}
