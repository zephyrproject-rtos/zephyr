/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <rpi_fw.h>

static const struct device *const hwinfo_firmware = DEVICE_DT_GET_ONE(raspberrypi_bcm283x_firmware);

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	/*
	 * The firmware reports a 64-bit serial. A zero response means the
	 * firmware did not write the data.
	 */
	uint32_t serial[2] = {0U, 0U};
	uint8_t id[sizeof(serial)];
	int ret;

	ret = rpi_fw_transfer(hwinfo_firmware, RPI_FW_TAG_GET_BOARD_SERIAL, serial, sizeof(serial));
	if (ret < 0) {
		return ret;
	}

	if ((serial[0] | serial[1]) == 0U) {
		return -ENODEV;
	}

	sys_put_be32(serial[1], id);
	sys_put_be32(serial[0], id + sizeof(serial[0]));

	length = MIN(length, sizeof(id));
	memcpy(buffer, id, length);

	return (ssize_t)length;
}

int z_impl_hwinfo_get_device_eui64(uint8_t *buffer)
{
	/* The firmware writes six MAC bytes and pads the response to a word. */
	uint8_t mac[8] = {0U};
	int ret;

	ret = rpi_fw_transfer(hwinfo_firmware, RPI_FW_TAG_GET_BOARD_MAC_ADDR, mac, sizeof(mac));
	if (ret < 0) {
		return ret;
	}

	if ((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0U) {
		/*
		 * The firmware did not write the response, which is what it does
		 * for a request size it does not expect.
		 */
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
