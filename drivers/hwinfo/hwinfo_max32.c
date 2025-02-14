/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <soc.h>
#include <mxc_sys.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	uint8_t usn[MXC_SYS_USN_LEN];
	uint8_t checksum[MXC_SYS_USN_CHECKSUM_LEN];
	int error;

	error = MXC_SYS_GetUSN(usn, checksum);
	if (error != E_NO_ERROR) {
		/* Error reading USN */
		return error;
	}

	if (length > sizeof(usn)) {
		length = sizeof(usn);
	}

	/* Provide device ID in big endian */
	sys_memcpy_swap(buffer, usn, length);

	return length;
}
