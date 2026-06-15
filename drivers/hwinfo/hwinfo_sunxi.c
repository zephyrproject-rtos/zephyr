/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Khai Do
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <errno.h>

#define CHIP_ID_LENGTH 16

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	const struct device *efuse = DEVICE_DT_GET(DT_NODELABEL(sid));
	uint8_t id[CHIP_ID_LENGTH];
	int err;

	if (!device_is_ready(efuse)) {
		return -ENODEV;
	}

	/* Read 128-bit Chip ID from offset 0x00 of eFuse */
	err = otp_read(efuse, 0x00, id, sizeof(id));
	if (err != 0) {
		return err;
	}

	length = MIN(length, sizeof(id));
	memcpy(buffer, id, length);

	return length;
}
