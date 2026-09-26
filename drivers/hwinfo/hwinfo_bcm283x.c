/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Raspberry Pi BCM283x hwinfo: returns the 64-bit board serial number
 * from VideoCore OTP via the firmware GET_BOARD_SERIAL property tag.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>

#include <rpi_fw.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	const struct device *fw = DEVICE_DT_GET_ONE(raspberrypi_bcm283x_firmware);
	uint8_t serial[8] = {0};
	int ret;

	ret = rpi_fw_transfer(fw, RPI_FW_TAG_GET_BOARD_SERIAL, serial, sizeof(serial));
	if (ret < 0) {
		return ret;
	}

	length = MIN(length, sizeof(serial));
	memcpy(buffer, serial, length);

	return (ssize_t)length;
}
