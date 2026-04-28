/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/byteorder.h>

/*
 * All BFLB SoCs (BL602, BL702, BL702L, BL616) store a factory-programmed
 * MAC address in efuse at offset 0x14 (low 32 bits) and 0x18 (high 32 bits,
 * only lower 16 bits used for MAC, bits 21:16 are parity). The MAC is stored
 * in little-endian order and must be byte-swapped to network (big-endian)
 * order so the OUI occupies the first three bytes.
 */
#define EFUSE_WIFI_MAC_LOW_OFFSET  0x14
#define EFUSE_WIFI_MAC_HIGH_OFFSET 0x18
#define DEVICE_ID_LENGTH           6

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	const struct device *efuse = DEVICE_DT_GET(DT_NODELABEL(efuse));
	uint32_t mac_low, mac_high;
	uint8_t id[DEVICE_ID_LENGTH];
	int err;

	if (!device_is_ready(efuse)) {
		return -ENODEV;
	}

	err = syscon_read_reg(efuse, EFUSE_WIFI_MAC_LOW_OFFSET, &mac_low);
	if (err != 0) {
		return err;
	}

	err = syscon_read_reg(efuse, EFUSE_WIFI_MAC_HIGH_OFFSET, &mac_high);
	if (err != 0) {
		return err;
	}

	/* Convert from efuse little-endian to network byte order */
	sys_put_be16((uint16_t)mac_high, &id[0]);
	sys_put_be32(mac_low, &id[2]);

	length = MIN(length, sizeof(id));
	memcpy(buffer, id, length);

	return length;
}
