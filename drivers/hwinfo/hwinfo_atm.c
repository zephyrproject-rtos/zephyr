/*
 * Copyright (c) 2023 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <string.h>

#include "calibration.h"

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
#ifdef CONFIG_SOC_SERIES_ATM33
#ifndef CONFIG_BT
#error "`CONFIG_BT=y` required for Atmosic hwinfo support"
#endif
	// Use Public BD Addr
	if (cal_pub_addr_len == sizeof(cal_pub_addr)) {
		if (length > cal_pub_addr_len) {
			length = cal_pub_addr_len;
		}

		memcpy(buffer, cal_pub_addr, length);
	} else {
		// Fallback to Default ID

		uint8_t const dft[] = {'l', 'o', 'w', 'p', 'w', 'r'};

		if (length > sizeof(dft)) {
			length = sizeof(dft);
		}

		memcpy(buffer, dft, length);
	}

	return length;
#else
	return -ENOSYS;
#endif
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	return -ENOSYS;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	return -ENOSYS;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	return -ENOSYS;
}
