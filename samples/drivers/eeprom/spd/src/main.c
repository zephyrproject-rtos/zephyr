/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/smbus.h>

#include <zephyr/sys/crc.h>

#include "spd.h"

/*
 * For reference: DDR5 Serial Presence Detect (SPD) Contents, JESD400-5A.01
 * Release 1.1
 */

static const char *get_memory_type(uint8_t type)
{
	switch (type) {
	case SPD_TYPE_DDR3:
		return "DDR3 SDRAM";
	case SPD_TYPE_DDR4:
		return "DDR4 SDRAM";
	case SPD_TYPE_LPDDR3:
		return "LPDDR3 SDRAM";
	case SPD_TYPE_LPDDR4:
		return "LPDDR4 SDRAM";
	case SPD_TYPE_DDR5:
		return "DDR5 SDRAM";
	default:
		return "Unknown";
	}
}

static int get_spd_length(uint8_t dev_descr)
{
	switch ((dev_descr >> 4) & 0b111) {
	case 0b001:
		return 256;
	case 0b010:
		return 512;
	case 0b011:
		return 1024;
	case 0b100:
		return 2048;
	default:
		return -EINVAL;
	}
}

static void spd_decode_lpddr4(const uint8_t *buf)
{
	printk("SPD Revision: %d.%d\n", (buf[SPD_REVISION] >> 4) & 0b1111,
	       (buf[SPD_REVISION] & 0b1111));

	printk("SPD Module Type: type %d media %d hybrid %d\n",
	       buf[SPD_MODULE_TYPE] & 0x0f,
	       (buf[SPD_MODULE_TYPE] >> 4) & 0x07,
	       (buf[SPD_MODULE_TYPE] >> 7) & 0x01);

	printk("SPD Area: Used %d bytes Total %d bytes\n",
	       (buf[SPD_DEV_DESCRIPTION] & 0b111) * 128,
	       ((buf[SPD_DEV_DESCRIPTION] >> 4) & 0b0111) * 256);

	printk("SPD SDRAM Ranks %d Width per channel %d\n",
	       ((buf[SPD_MODULE_ORGANIZATION] >> 3) & 0x07) + 1,
	       0x4 << (buf[SPD_MODULE_ORGANIZATION] & 0x0f));

	printk("SPD SDRAM System Channel Bus Width %d\n",
	       0x8 << (buf[SPD_MODULE_MEMORY_BUS_WIDTH] & 0x07));

	printk("SPD CRC16 LSB 0x%x MSB 0x%x\n",
	       buf[SPD4_CRC16_LSB], buf[SPD4_CRC16_MSB]);
}

static const char *get_manufacturer(uint16_t id)
{
	/**
	 * The table would be populated when needed. The code is 2 bytes,
	 * page and actual id. It also has odd parity bit. At the moment
	 * very simplified.
	 *
	 * See Jep106.
	 */
	switch (id) {
	case 0x802c:
		return "Micron";
	case 0x80ce:
		return "Samsung";
	default:
		return "Unknown";
	}
}

static void spd_decode_ddr5(const uint8_t *buf)
{
	uint16_t id, crc;

	printk("SPD Revision: %d.%d\n", (buf[SPD_REVISION] >> 4) & 0b1111,
	       (buf[SPD_REVISION] & 0b1111));

	id = buf[512] << 8 | buf[513];
	printk("Memory manufacturer: (0x%04x) %s\n", id, get_manufacturer(id));

	crc = buf[SPD5_CRC16_MSB] << 8 | buf[SPD5_CRC16_LSB];
	printk("SPD CRC16 0x%04x\n", crc);

	/*
	 * Calculate CRC16 with fast recommended function crc16_itu_t()
	 * Verify that the 16-bit cyclic redundancy check (CRC) for bytes 0 to 509
	 * matches the value stored.
	 */
	if (crc == crc16_itu_t(0, buf, 510)) {
		printk("CRC16 verified\n");
	} else {
		printk("CRC16 does not match with calculated value\n");
	}
}

static void spd_eeprom_decode(const uint8_t *buf, size_t len)
{
	uint8_t dev_descr = buf[SPD_DEV_DESCRIPTION];
	uint8_t memory_type = buf[SPD_MEMORY_TYPE];
	int spd_len = get_spd_length(dev_descr);

	if (spd_len < 0 || spd_len > len) {
		return;
	}

	printk("SPD length: %d\n", spd_len);
	printk("SPD Memory Type: %s\n", get_memory_type(memory_type));

	switch (memory_type) {
	case SPD_TYPE_LPDDR4:
		return spd_decode_lpddr4(buf);
	case SPD_TYPE_DDR5:
		return spd_decode_ddr5(buf);
	default:
		printk("Memory type %s (%u) is not decoded\n",
		       get_memory_type(memory_type), memory_type);
	}
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(eeprom_spd));
	uint8_t spd_buf[1024];
	size_t len;
	int ret;

	if (!device_is_ready(dev)) {
		printk("EEPROM SPD device not found\n");
		return -ENODEV;
	}

	printk("Start SPD EEPROM sample %s\n", CONFIG_BOARD);

	len = eeprom_get_size(dev);

	printk("EEPROM size %zd\n", len);

	if (len > sizeof(spd_buf)) {
		return -ENODEV;
	}

	ret = eeprom_read(dev, 0, spd_buf, len);
	if (ret) {
		printk("Cannot read SPD EEPROM");
		return -EINVAL;
	}

	spd_eeprom_decode(spd_buf, len);

	return 0;
}
