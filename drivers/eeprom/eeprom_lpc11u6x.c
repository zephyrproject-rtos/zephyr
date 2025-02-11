/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc11u6x_eeprom

/**
 * @file
 * @brief EEPROM driver for NXP LPC11U6X MCUs
 *
 * This driver supports the on-chip EEPROM found on NXP LPC11U6x MCUs.
 *
 * @note This driver is only a wrapper for the IAP (In-Application Programming)
 *       EEPROM functions.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/eeprom.h>
#include <iap.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_lpc11u6x);

struct eeprom_lpc11u6x_config {
	size_t size;
};

static int eeprom_lpc11u6x_read(const struct device *dev,
				off_t offset, void *data, size_t len)
{
	const struct eeprom_lpc11u6x_config *config = dev->config;
	uint32_t cmd[5];
	int ret;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	cmd[0] = IAP_CMD_EEPROM_READ;
	cmd[1] = offset;
	cmd[2] = (uint32_t) data;
	cmd[3] = len;
	cmd[4] = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000;

	ret = iap_cmd(cmd);

	if (ret != IAP_STATUS_CMD_SUCCESS) {
		LOG_ERR("failed to read EEPROM (offset=%08x len=%d err=%d)",
			(unsigned int) offset, len, ret);
		return -EINVAL;
	}

	return 0;
}

static int eeprom_lpc11u6x_write(const struct device *dev,
				 off_t offset, const void *data, size_t len)
{
	const struct eeprom_lpc11u6x_config *config = dev->config;
	uint32_t cmd[5];
	int ret;

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	cmd[0] = IAP_CMD_EEPROM_WRITE;
	cmd[1] = offset;
	cmd[2] = (uint32_t) data;
	cmd[3] = len;
	cmd[4] = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000;

	ret = iap_cmd(cmd);

	if (ret != IAP_STATUS_CMD_SUCCESS) {
		LOG_ERR("failed to write EEPROM (offset=%08x len=%d err=%d)",
			(unsigned int) offset, len, ret);
		return -EINVAL;
	}

	return 0;
}

static size_t eeprom_lpc11u6x_size(const struct device *dev)
{
	const struct eeprom_lpc11u6x_config *config = dev->config;

	return config->size;
}

static const struct eeprom_driver_api eeprom_lpc11u6x_api = {
	.read = eeprom_lpc11u6x_read,
	.write = eeprom_lpc11u6x_write,
	.size = eeprom_lpc11u6x_size,
};

static const struct eeprom_lpc11u6x_config eeprom_config = {
	.size = DT_INST_PROP(0, size),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &eeprom_config, POST_KERNEL,
		      CONFIG_EEPROM_INIT_PRIORITY, &eeprom_lpc11u6x_api);
