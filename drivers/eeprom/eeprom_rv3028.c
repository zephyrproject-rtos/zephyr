/*
 * Copyright (c) 2026 Janez Ugovsek <janez@ugovsek.info>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <rv3028.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT microcrystal_rv3028_eeprom

LOG_MODULE_REGISTER(eeprom_rv3028, CONFIG_EEPROM_LOG_LEVEL);

struct rv3028_config {
	const struct device *mfd;
};

static size_t rv3028_eeprom_size(const struct device *dev)
{
	ARG_UNUSED(dev);

	return RV3028_EEPROM_SIZE;
}

static int rv3028_eeprom_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct rv3028_config *config = dev->config;
	const uint8_t *data = buf;
	int ret = 0;

	if ((offset < 0) || ((offset + len) > RV3028_EEPROM_SIZE)) {
		LOG_WRN("EEPROM write out of range");
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	mfd_rv3028_lock_sem(config->mfd);
	ret = mfd_rv3028_enter_eerd(config->mfd);
	if (ret) {
		mfd_rv3028_unlock_sem(config->mfd);
		return ret;
	}

	for (size_t i = 0; i < len; i++) {
		ret = mfd_rv3028_write_reg8(config->mfd, RV3028_REG_EEPROM_ADDRESS, offset + i);
		if (ret) {
			LOG_WRN("Cannot set EEPROM address");
			ret = -EIO;
			goto unlock;
		}

		ret = mfd_rv3028_write_reg8(config->mfd, RV3028_REG_EEPROM_DATA, data[i]);
		if (ret) {
			LOG_WRN("Cannot set EEPROM data");
			ret = -EIO;
			goto unlock;
		}

		ret = mfd_rv3028_eeprom_command(config->mfd, RV3028_EEPROM_CMD_WRITE);
		if (ret) {
			LOG_WRN("Cannot set EEPROM write command");
			ret = -EIO;
			goto unlock;
		}

		ret = mfd_rv3028_eeprom_wait_busy(config->mfd, RV3028_EEBUSY_WRITE_POLL_MS);
		if (ret) {
			LOG_WRN("EEPROM write command timed out");
			ret = -EIO;
			goto unlock;
		}
	}

unlock:
	mfd_rv3028_exit_eerd(config->mfd);
	mfd_rv3028_unlock_sem(config->mfd);
	return ret;
}

static int rv3028_eeprom_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct rv3028_config *config = dev->config;
	uint8_t *data = buf;
	int ret = 0;

	if ((offset < 0) || ((offset + len) > RV3028_EEPROM_SIZE)) {
		LOG_WRN("EEPROM read out of range");
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	mfd_rv3028_lock_sem(config->mfd);
	ret = mfd_rv3028_enter_eerd(config->mfd);
	if (ret) {
		mfd_rv3028_unlock_sem(config->mfd);
		return ret;
	}

	for (size_t i = 0; i < len; i++) {
		ret = mfd_rv3028_write_reg8(config->mfd, RV3028_REG_EEPROM_ADDRESS, offset + i);
		if (ret) {
			LOG_WRN("Cannot set EEPROM address");
			ret = -EIO;
			goto unlock;
		}

		ret = mfd_rv3028_eeprom_command(config->mfd, RV3028_EEPROM_CMD_READ);
		if (ret) {
			LOG_WRN("Cannot set EEPROM read command");
			ret = -EIO;
			goto unlock;
		}

		ret = mfd_rv3028_eeprom_wait_busy(config->mfd, RV3028_EEBUSY_READ_POLL_MS);
		if (ret) {
			LOG_WRN("EEPROM read command timed out");
			ret = -EIO;
			goto unlock;
		}

		ret = mfd_rv3028_read_reg8(config->mfd, RV3028_REG_EEPROM_DATA, &data[i]);
		if (ret) {
			LOG_WRN("Cannot read EEPROM data");
			ret = -EIO;
			goto unlock;
		}
	}

unlock:
	mfd_rv3028_exit_eerd(config->mfd);
	mfd_rv3028_unlock_sem(config->mfd);
	return ret;
}

static DEVICE_API(eeprom, rv3028_driver_api) = {
	.read = rv3028_eeprom_read,
	.write = rv3028_eeprom_write,
	.size = rv3028_eeprom_size,
};

#define INIT(inst)                                                                                 \
	static const struct rv3028_config rv3028_config_##inst = {                                 \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &rv3028_config_##inst, POST_KERNEL,          \
			      CONFIG_EEPROM_INIT_PRIORITY, &rv3028_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INIT)
