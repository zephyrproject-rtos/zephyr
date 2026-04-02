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
 * MAC address in efuse at offset 0x14 (low 32 bits) and 0x18. The MAC is stored
 * in little-endian order and must be byte-swapped to network (big-endian)
 * order so the OUI occupies the first three bytes.
 */
#define EFUSE_WIFI_MAC_LOW_OFFSET	0x14
#define EFUSE_WIFI_MAC_HIGH_OFFSET	0x18
/* BL70x/L parity slot */
#define EFUSE_DATA_0_EF_KEY_SLOT_5_W2	0x74

#define DEVICE_ID_LENGTH		6
#define DEVICE_ID_LENGTH_EFUSE		8

#define EFUSE_WIFI_MAC_PARITY_MSK	0x3f
#define EFUSE_WIFI_MAC_PARITY_POS	16

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	const struct device *efuse = DEVICE_DT_GET(DT_NODELABEL(efuse));
	uint32_t mac_low, mac_high;
	uint8_t id[DEVICE_ID_LENGTH_EFUSE];
	uint8_t parity_cl, parity_ef;
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

	/* Copy and convert from efuse little-endian to network byte order */
#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
	/* Keep whole length: 0s are counted */
	sys_put_be32(mac_high, &id[0]);
	sys_put_be32(mac_low, &id[4]);
#else
	sys_put_be16((uint16_t)mac_high, &id[0]);
	sys_put_be32(mac_low, &id[2]);
#endif

	/* Check parity which uses 0 count */
#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL70XL)
	parity_cl =  (DEVICE_ID_LENGTH_EFUSE * BITS_PER_BYTE)
		- (sys_count_bits(id, DEVICE_ID_LENGTH_EFUSE) & EFUSE_WIFI_MAC_PARITY_MSK);
	/* BL70x/L stores parity elsewhere */
	err = syscon_read_reg(efuse, EFUSE_DATA_0_EF_KEY_SLOT_5_W2, &mac_high);
	if (err != 0) {
		return err;
	}
	parity_ef = mac_high & EFUSE_WIFI_MAC_PARITY_MSK;
	/* Keep only the bits we want after parity check */
	sys_put_be16((uint16_t)(mac_low >> 16), &id[4]);
#else
	parity_cl =  (DEVICE_ID_LENGTH * BITS_PER_BYTE)
		- (sys_count_bits(id, DEVICE_ID_LENGTH) & EFUSE_WIFI_MAC_PARITY_MSK);
	parity_ef = (mac_high >> EFUSE_WIFI_MAC_PARITY_POS) & EFUSE_WIFI_MAC_PARITY_MSK;
#endif

	if (parity_cl != parity_ef) {
		return -EIO;
	}

	length = MIN(length, DEVICE_ID_LENGTH);
	memcpy(buffer, id, length);

	return length;
}
