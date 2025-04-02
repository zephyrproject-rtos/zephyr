/*
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <ti/driverlib/driverlib.h>

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t reason = DL_SYSCTL_getResetCause();

	switch (reason) {
	case DL_SYSCTL_RESET_CAUSE_POR_HW_FAILURE:
		*cause = RESET_POR;
		break;
	case DL_SYSCTL_RESET_CAUSE_POR_EXTERNAL_NRST:
		__fallthrough;
	case DL_SYSCTL_RESET_CAUSE_BOOTRST_EXTERNAL_NRST:
		*cause = RESET_PIN;
		break;
	case DL_SYSCTL_RESET_CAUSE_POR_SW_TRIGGERED:
		__fallthrough;
	case DL_SYSCTL_RESET_CAUSE_BOOTRST_SW_TRIGGERED:
		__fallthrough;
	case DL_SYSCTL_RESET_CAUSE_SYSRST_SW_TRIGGERED:
		__fallthrough;
	case DL_SYSCTL_RESET_CAUSE_CPURST_SW_TRIGGERED:
		*cause = RESET_SOFTWARE;
		break;
	case DL_SYSCTL_RESET_CAUSE_BOR_SUPPLY_FAILURE:
		*cause = RESET_BROWNOUT;
		break;
	case DL_SYSCTL_RESET_CAUSE_BOR_WAKE_FROM_SHUTDOWN:
		*cause = RESET_LOW_POWER_WAKE;
		break;
	case DL_SYSCTL_RESET_CAUSE_BOOTRST_NON_PMU_PARITY_FAULT:
		*cause = RESET_PARITY;
		break;
	case DL_SYSCTL_RESET_CAUSE_BOOTRST_CLOCK_FAULT:
		*cause = RESET_CLOCK;
		break;
	case DL_SYSCTL_RESET_CAUSE_SYSRST_BSL_EXIT:
		__fallthrough;
	case DL_SYSCTL_RESET_CAUSE_SYSRST_BSL_ENTRY:
		*cause = RESET_BOOTLOADER;
		break;
	case DL_SYSCTL_RESET_CAUSE_SYSRST_WWDT0_VIOLATION:
		__fallthrough;
	case DL_SYSCTL_RESET_CAUSE_SYSRST_WWDT1_VIOLATION:
		*cause = RESET_WATCHDOG;
		break;
	case DL_SYSCTL_RESET_CAUSE_SYSRST_FLASH_ECC_ERROR:
		*cause = RESET_FLASH;
		break;
	case DL_SYSCTL_RESET_CAUSE_SYSRST_CPU_LOCKUP_VIOLATION:
		*cause = RESET_CPU_LOCKUP;
		break;
	case DL_SYSCTL_RESET_CAUSE_SYSRST_DEBUG_TRIGGERED:
		__fallthrough;
	case DL_SYSCTL_RESET_CAUSE_CPURST_DEBUG_TRIGGERED:
		*cause = RESET_DEBUG;
		break;
	default:
		break;
	}

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = RESET_POR
		   | RESET_PIN
		   | RESET_SOFTWARE
		   | RESET_BROWNOUT
		   | RESET_LOW_POWER_WAKE
		   | RESET_PARITY
		   | RESET_CLOCK
		   | RESET_BOOTLOADER
		   | RESET_WATCHDOG
		   | RESET_FLASH
		   | RESET_CPU_LOCKUP
		   | RESET_DEBUG;

	return 0;
}
