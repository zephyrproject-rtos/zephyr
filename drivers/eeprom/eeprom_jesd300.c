/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Driver for DDR5 Serial Presence Detect (SPD) EEPROM devices that are
 * compatible with the JEDEC Standard No. 300-5 (JESD300-5) specification.
 *
 * Supports SPD5 Hub versions: SPD5118 and SPD5108.
 */

#define DT_DRV_COMPAT jedec_jesd300

#include <zephyr/kernel.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/smbus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(jesd300_eeprom, CONFIG_EEPROM_LOG_LEVEL);

/* Register description */

#define JESD300_PAGE_SIZE	128
#define JESD300_PAGE_SHIFT	7
#define JESD300_PAGE_MASK	(BIT(JESD300_PAGE_SHIFT) - 1)

#define JESD300_MR0		0x00 /* Device Type: Most Significant Byte */
#define MR0_TYPE_SPD5_HUB	0x51 /* SPD5 Hub Device */

#define JESD300_MR1		0x01 /* Device Type: Least Significant Byte */

#define JESD300_MR11		0x0b /* I2C Legacy Mode Configuration */

struct eeprom_config {
	struct smbus_dt_spec smbus_spec;
	size_t size;
};

struct eeprom_data {
	uint8_t current_page;
};

static bool check_eeprom_bounds(const struct device *dev, off_t offset,
				size_t len)
{
	const struct eeprom_config *config = dev->config;

	if (offset + len > config->size) {
		return false;
	}

	return true;
}

static size_t size(const struct device *dev)
{
	const struct eeprom_config *config = dev->config;

	return config->size;
}

static int write(const struct device *dev, off_t offset, const void *data,
		 size_t len)
{
	if (!len) {
		return 0;
	}

	if (!check_eeprom_bounds(dev, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("offset 0x%tx len %zd", (ptrdiff_t)offset, len);

	/* Do not write to SPD5 EEPROM */
	ARG_UNUSED(data);

	return -ENOTSUP;
}

static int read_byte(const struct device *dev, off_t offset,
		     uint8_t *data)
{
	const struct eeprom_config *config = dev->config;
	struct eeprom_data *dev_data = dev->data;
	uint8_t page = offset >> JESD300_PAGE_SHIFT;
	int ret;

	/**
	 * Switch page if needed, at the moment the other fields of the MR11
	 * register are zeroes, so just write page number.
	 */
	if (page != dev_data->current_page) {
		ret = smbus_byte_data_write(config->smbus_spec.bus,
					    config->smbus_spec.addr,
					    JESD300_MR11, page);
		if (ret < 0) {
			LOG_ERR("Changing page failed (%d)", ret);
			return ret;
		}

		LOG_DBG("Change page %u -> %u", dev_data->current_page, page);

		dev_data->current_page = page;
	}

	offset &= JESD300_PAGE_MASK;
	offset = offset | 0x80;

	return smbus_byte_data_read(config->smbus_spec.bus, config->smbus_spec.addr,
				    offset, data);
}

static int read(const struct device *dev, off_t offset, void *data,
		size_t len)
{
	uint8_t *p = data;

	if (!len) {
		return 0;
	}

	if (!check_eeprom_bounds(dev, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("offset 0x%tx len %zd", (ptrdiff_t)offset, len);

	/**
	 * Using simple byte read allows to cross page boundaries,
	 * TODO: Check block read possibility
	 */
	for (size_t i = 0; i < len; i++) {
		read_byte(dev, offset + i, p + i);
	}

	return 0;
}

static int eeprom_init(const struct device *dev)
{
	const struct eeprom_config *config = dev->config;
	uint8_t data;
	int ret;

	if (!device_is_ready(config->smbus_spec.bus)) {
		LOG_ERR("Error initializing JESD300 device");
		return -ENODEV;
	}

	/**
	 * By default device accept 1 byte of address which covers first
	 * 128 bytes of memory. Check that we have correct device.
	 */
	ret = smbus_byte_data_read(config->smbus_spec.bus,
				   config->smbus_spec.addr,
				   JESD300_MR0, &data);
	if (ret < 0 || data != MR0_TYPE_SPD5_HUB) {
		LOG_ERR("Incompatible EEPROM SPD");
		return -ENOTSUP;
	}

	ret = smbus_byte_data_read(config->smbus_spec.bus,
				   config->smbus_spec.addr,
				   JESD300_MR1, &data);
	if (ret < 0) {
		LOG_ERR("Read failed");
		return -ENOTSUP;
	}

	switch (data) {
	case 0x18:
		LOG_INF("Detected SPD5 Hub Device with Temp Sensor");
		break;
	case 0x08:
		LOG_INF("Detected SPD5 Hub Device without Temp Sensor");
		break;
	default:
		LOG_ERR("Unknown device (0x%x)", data);
		return -ENOTSUP;
	}

	LOG_INF("Jedec JESD300 SMBus EEPROM driver initialized");

	return 0;
}

static const struct eeprom_driver_api eeprom_api = {
	.read = read,
	.write = write,
	.size = size,
};

BUILD_ASSERT(CONFIG_EEPROM_INIT_PRIORITY > CONFIG_SMBUS_INIT_PRIORITY);

#define DEFINE_SMBUS_EEPROM(n)                                                 \
	static const struct eeprom_config eeprom_config##n = {                 \
		.size = DT_INST_PROP(n, size),                                 \
		.smbus_spec = SMBUS_DT_SPEC_INST_GET(n)                        \
	};                                                                     \
	static struct eeprom_data eeprom_data##n;                              \
	DEVICE_DT_INST_DEFINE(n, eeprom_init, NULL,                            \
			      &eeprom_data##n, &eeprom_config##n,              \
			      POST_KERNEL, CONFIG_EEPROM_INIT_PRIORITY,        \
			      &eeprom_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SMBUS_EEPROM);
