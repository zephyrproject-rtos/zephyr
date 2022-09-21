/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_smbus_eeprom

#include <zephyr/kernel.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/smbus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smbus_eeprom, CONFIG_EEPROM_LOG_LEVEL);

struct eeprom_config {
	struct smbus_dt_spec smbus_spec;
	size_t size;
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
	const struct eeprom_config *config = dev->config;
	const uint8_t *p = data;
	uint8_t off;

	if (!len) {
		return 0;
	}

	if (!check_eeprom_bounds(dev, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("offset 0x%tx len %zd", (ptrdiff_t)offset, len);

	/* TODO: Paging, for now assume offset 1 byte */
	off = (uint8_t)offset;

	for (int i = 0; i < len; i++) {
		smbus_byte_data_write(config->smbus_spec.bus,
				      config->smbus_spec.addr,
				      off + i, p[i]);
	}

	return 0;
}

static int read(const struct device *dev, off_t offset, void *data,
		size_t len)
{
	const struct eeprom_config *config = dev->config;
	uint8_t *p = data;
	uint8_t off;

	if (!len) {
		return 0;
	}

	if (!check_eeprom_bounds(dev, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("offset 0x%tx len %zd", (ptrdiff_t)offset, len);

	/* TODO: Paging, for now assume offset 1 byte */
	off = (uint8_t)offset;

	for (int i = 0; i < len; i++) {
		smbus_byte_data_read(config->smbus_spec.bus,
				     config->smbus_spec.addr,
				     off + i, p + i);
	}

	return 0;
}

static int eeprom_init(const struct device *dev)
{
	const struct eeprom_config *config = dev->config;

	if (!device_is_ready(config->smbus_spec.bus)) {
		return -ENODEV;
	}

	LOG_INF("SMBus EEPROM driver initialized");

	return 0;
}

static const struct eeprom_driver_api eeprom_api = {
	.read = read,
	.write = write,
	.size = size,
};

#define DEFINE_SMBUS_EEPROM(n)                                                 \
	static const struct eeprom_config eeprom_config##n = {                 \
		.size = DT_INST_PROP(n, size),                                 \
		.smbus_spec = SMBUS_DT_SPEC_INST_GET(n)                        \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(n, eeprom_init, NULL,                            \
			      NULL, &eeprom_config##n,                         \
			      POST_KERNEL, CONFIG_EEPROM_INIT_PRIORITY,        \
			      &eeprom_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SMBUS_EEPROM);
