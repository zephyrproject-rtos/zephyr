/*
 * Copyright (c) 2024 ANITRA system s.r.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3028_eeprom

#include <zephyr/drivers/eeprom.h>
#include <zephyr/kernel.h>
#include "rtc_rv3028.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(rv3028, CONFIG_EEPROM_LOG_LEVEL);

#define RV3028_USER_EEPROM_SIZE 0x2B

struct rv3028_eeprom_config {
	const struct device *parent;
	uint8_t addr;
	uint8_t size;
};

static size_t rv3028_eeprom_get_size(const struct device *dev)
{
	const struct rv3028_eeprom_config *config = dev->config;

	return config->size;
}

static int rv3028_eeprom_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct rv3028_eeprom_config *config = dev->config;
	const struct device *rtc_dev = config->parent;
	uint8_t *dst = data;
	uint8_t addr;
	int err;

	if (!len) {
		return 0;
	}

	if (offset + len > config->size) {
		LOG_ERR("attempt to read past device boundary");
		return -EINVAL;
	}

	rv3028_lock_sem(rtc_dev);

	err = rv3028_enter_eerd(rtc_dev);
	if (err) {
		goto unlock;
	}

	for (size_t i = 0; i < len; i++) {
		addr = config->addr + offset + i;

		err = rv3028_write_reg8(rtc_dev, RV3028_REG_EEPROM_ADDRESS, addr);
		if (err) {
			goto unlock;
		}

		err = rv3028_eeprom_command(rtc_dev, RV3028_EEPROM_CMD_READ);
		if (err) {
			goto unlock;
		}

		k_msleep(RV3028_EEBUSY_READ_POLL_MS);

		err = rv3028_eeprom_wait_busy(rtc_dev, RV3028_EEBUSY_READ_POLL_MS);
		if (err) {
			goto unlock;
		}

		err = rv3028_read_reg8(rtc_dev, RV3028_REG_EEPROM_DATA, &dst[i]);
		if (err) {
			goto unlock;
		}
	}

unlock:
	rv3028_exit_eerd(rtc_dev);
	rv3028_unlock_sem(rtc_dev);

	return err;
}

static int rv3028_eeprom_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct rv3028_eeprom_config *config = dev->config;
	const struct device *rtc_dev = config->parent;
	const uint8_t *src = data;
	uint8_t cmd[2];
	int err;

	if (!len) {
		return 0;
	}

	if (offset + len > config->size) {
		LOG_ERR("attempt to write past device boundary");
		return -EINVAL;
	}

	rv3028_lock_sem(rtc_dev);

	err = rv3028_enter_eerd(rtc_dev);
	if (err) {
		goto unlock;
	}

	for (size_t i = 0; i < len; i++) {
		cmd[0] = config->addr + offset + i;
		cmd[1] = src[i];

		err = rv3028_write_regs(rtc_dev, RV3028_REG_EEPROM_ADDRESS, cmd, sizeof(cmd));
		if (err) {
			goto unlock;
		}

		err = rv3028_eeprom_command(rtc_dev, RV3028_EEPROM_CMD_WRITE);
		if (err) {
			goto unlock;
		}

		k_msleep(RV3028_EEBUSY_WRITE_POLL_MS);

		err = rv3028_eeprom_wait_busy(rtc_dev, RV3028_EEBUSY_WRITE_POLL_MS);
		if (err) {
			goto unlock;
		}
	}

unlock:
	rv3028_exit_eerd(rtc_dev);
	rv3028_unlock_sem(rtc_dev);

	return err;
}

static int rv3028_eeprom_init(const struct device *dev)
{
	const struct rv3028_eeprom_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("parent device %s is not ready", config->parent->name);
		return -ENODEV;
	}

	return 0;
}

static const struct eeprom_driver_api rv3028_eeprom_api = {
	.size = rv3028_eeprom_get_size,
	.read = rv3028_eeprom_read,
	.write = rv3028_eeprom_write,
};

#define RV3028_EEPROM_ASSERT_AREA_SIZE(inst)                                                       \
	BUILD_ASSERT(DT_INST_REG_SIZE(inst) + DT_INST_REG_ADDR(inst) <= RV3028_USER_EEPROM_SIZE,   \
		     "Invalid RV3028 EEPROM area size");

#define RV3028_EEPROM_DEFINE(inst)                                                                 \
	RV3028_EEPROM_ASSERT_AREA_SIZE(inst)                                                       \
                                                                                                   \
	static const struct rv3028_eeprom_config rv3028_eeprom_config_##inst = {                   \
		.parent = DEVICE_DT_GET(DT_INST_BUS(inst)),                                        \
		.addr = DT_INST_REG_ADDR(inst),                                                    \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, rv3028_eeprom_init, NULL, NULL, &rv3028_eeprom_config_##inst,  \
			      POST_KERNEL, CONFIG_EEPROM_RV3028_INIT_PRIORITY, &rv3028_eeprom_api);

DT_INST_FOREACH_STATUS_OKAY(RV3028_EEPROM_DEFINE);
