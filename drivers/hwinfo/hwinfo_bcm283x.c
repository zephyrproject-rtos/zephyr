/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Raspberry Pi BCM283x hwinfo: returns the 64-bit board serial number
 * from VideoCore OTP via the firmware GET_BOARD_SERIAL property tag.
 */

#include <errno.h>
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

	if (!device_is_ready(fw)) {
		return -ENODEV;
	}

	ret = rpi_fw_transfer(fw, RPI_FW_TAG_GET_BOARD_SERIAL, serial, sizeof(serial));
	if (ret < 0) {
		return ret;
	}

	length = MIN(length, sizeof(serial));
	memcpy(buffer, serial, length);

	return (ssize_t)length;
}

int z_impl_hwinfo_get_device_eui64(uint8_t *buffer)
{
	const struct device *fw = DEVICE_DT_GET_ONE(raspberrypi_bcm283x_firmware);
	/* The six-byte MAC response is padded to eight bytes. */
	uint8_t mac[8] = {0U};
	int ret;

	if (!device_is_ready(fw)) {
		return -ENODEV;
	}

	ret = rpi_fw_transfer(fw, RPI_FW_TAG_GET_BOARD_MAC_ADDR, mac, sizeof(mac));
	if (ret < 0) {
		return ret;
	}

	if ((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0U) {
		return -ENODEV;
	}

	/* IEEE EUI-64 formed from the 48-bit MAC address. */
	buffer[0] = mac[0];
	buffer[1] = mac[1];
	buffer[2] = mac[2];
	buffer[3] = 0xffU;
	buffer[4] = 0xfeU;
	buffer[5] = mac[3];
	buffer[6] = mac[4];
	buffer[7] = mac[5];

	return 0;
}
