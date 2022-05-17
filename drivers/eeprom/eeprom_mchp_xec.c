/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_eeprom

#include <zephyr/drivers/eeprom.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_xec);

K_MUTEX_DEFINE(lock);

struct eeprom_xec_config {
	uint32_t addr;
	size_t size;
};

static int eeprom_xec_read(const struct device *dev, off_t offset,
				void *buf,
				size_t len)
{
	const struct eeprom_xec_config *config = dev->config;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	/* EEPROM HW READ */

	k_mutex_unlock(&lock);

	return 0;
}

static int eeprom_xec_write(const struct device *dev, off_t offset,
				const void *buf, size_t len)
{
	const struct eeprom_xec_config *config = dev->config;
	int ret = 0;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	/* EEPROM HW WRITE */

	k_mutex_unlock(&lock);

	return ret;
}

static size_t eeprom_xec_size(const struct device *dev)
{
	const struct eeprom_xec_config *config = dev->config;

	return config->size;
}

static int eeprom_xec_init(const struct device *dev)
{
	return 0;
}

static const struct eeprom_driver_api eeprom_xec_api = {
	.read = eeprom_xec_read,
	.write = eeprom_xec_write,
	.size = eeprom_xec_size,
};

static const struct eeprom_xec_config eeprom_config = {
	.addr = DT_INST_REG_ADDR(0),
	.size = DT_INST_REG_SIZE(0),
};

DEVICE_DT_INST_DEFINE(0, &eeprom_xec_init, NULL, NULL,
		    &eeprom_config, POST_KERNEL,
		    CONFIG_EEPROM_INIT_PRIORITY, &eeprom_xec_api);
