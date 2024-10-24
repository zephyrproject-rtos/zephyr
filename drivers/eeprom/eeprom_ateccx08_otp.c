/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for Microchip ATECCX08 I2C EEPROMs.
 */

#define DT_DRV_COMPAT microchip_ateccx08_otp

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mfd/ateccx08.h>
#include <zephyr/drivers/eeprom.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ateccx08_otp, CONFIG_EEPROM_LOG_LEVEL);

struct eeprom_ateccx08_config {
	const struct device *parent;
	bool readonly;
};

static size_t eeprom_ateccx08_size(const struct device *dev)
{
	ARG_UNUSED(dev);

	return atecc_get_zone_size(ATECC_ZONE_OTP, 0);
}

static int eeprom_ateccx08_write(const struct device *dev, off_t offset, const void *data,
				 size_t len)
{
	const struct eeprom_ateccx08_config *cfg = dev->config;

	if (cfg->readonly) {
		LOG_ERR("attempt to write to read-only device");
		return -EACCES;
	}

	return atecc_write_bytes(cfg->parent, ATECC_ZONE_OTP, 0, offset, data, len);
}

static int eeprom_ateccx08_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct eeprom_ateccx08_config *cfg = dev->config;

	return atecc_read_bytes(cfg->parent, ATECC_ZONE_OTP, 0, offset, data, len);
}

static int eeprom_ateccx08_init(const struct device *dev)
{
	const struct eeprom_ateccx08_config *cfg = dev->config;

	if (!device_is_ready(cfg->parent)) {
		return -ENODEV;
	}

	return 0;
}

static const struct eeprom_driver_api eeprom_ateccx08_driver_api = {
	.read = &eeprom_ateccx08_read,
	.write = &eeprom_ateccx08_write,
	.size = &eeprom_ateccx08_size,
};

BUILD_ASSERT(CONFIG_EEPROM_ATECCX08_OTP_INIT_PRIORITY >= CONFIG_MFD_ATECCX08_INIT_PRIORITY,
	     "ATECCX08 EEPROM driver must be initialized after the mfd driver");

#define DEFINE_ATECCX08_OTP(_num)                                                                  \
	static const struct eeprom_ateccx08_config eeprom_ateccx08_config##_num = {                \
		.parent = DEVICE_DT_GET(DT_INST_BUS(_num)),                                        \
		.readonly = DT_INST_PROP(_num, read_only)};                                        \
	DEVICE_DT_INST_DEFINE(_num, eeprom_ateccx08_init, NULL, NULL,                              \
			      &eeprom_ateccx08_config##_num, POST_KERNEL,                          \
			      CONFIG_EEPROM_ATECCX08_OTP_INIT_PRIORITY,                            \
			      &eeprom_ateccx08_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_ATECCX08_OTP);
