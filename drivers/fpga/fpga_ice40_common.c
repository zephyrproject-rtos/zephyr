/*
 * Copyright (c) 2022 Meta
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "fpga_ice40_common.h"

LOG_MODULE_REGISTER(fpga_ice40);

void fpga_ice40_crc_to_str(uint32_t crc, char *s)
{
	char ch;
	uint8_t i;
	uint8_t nibble;
	const char *table = "0123456789abcdef";

	for (i = 0; i < sizeof(crc) * NIBBLES_PER_BYTE; ++i, crc >>= BITS_PER_NIBBLE) {
		nibble = crc & GENMASK(BITS_PER_NIBBLE, 0);
		ch = table[nibble];
		s[sizeof(crc) * NIBBLES_PER_BYTE - i - 1] = ch;
	}

	s[sizeof(crc) * NIBBLES_PER_BYTE] = '\0';
}

enum FPGA_status fpga_ice40_get_status(const struct device *dev)
{
	enum FPGA_status st;
	k_spinlock_key_t key;
	struct fpga_ice40_data *data = dev->data;

	key = k_spin_lock(&data->lock);

	if (data->loaded && data->on) {
		st = FPGA_STATUS_ACTIVE;
	} else {
		st = FPGA_STATUS_INACTIVE;
	}

	k_spin_unlock(&data->lock, key);

	return st;
}

static int fpga_ice40_on_off(const struct device *dev, bool on)
{
	int ret;
	k_spinlock_key_t key;
	struct fpga_ice40_data *data = dev->data;
	const struct fpga_ice40_config *config = dev->config;

	key = k_spin_lock(&data->lock);

	ret = gpio_pin_configure_dt(&config->creset, on ? GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW);
	if (ret < 0) {
		goto unlock;
	}

	data->on = on;
	ret = 0;

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

int fpga_ice40_on(const struct device *dev)
{
	return fpga_ice40_on_off(dev, true);
}

int fpga_ice40_off(const struct device *dev)
{
	return fpga_ice40_on_off(dev, false);
}

int fpga_ice40_reset(const struct device *dev)
{
	return fpga_ice40_off(dev) || fpga_ice40_on(dev);
}

const char *fpga_ice40_get_info(const struct device *dev)
{
	struct fpga_ice40_data *data = dev->data;

	return data->info;
}

int fpga_ice40_init(const struct device *dev)
{
	int ret;
	const struct fpga_ice40_config *config = dev->config;

	if (!device_is_ready(config->creset.port)) {
		LOG_ERR("%s: GPIO for creset is not ready", dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->cdone.port)) {
		LOG_ERR("%s: GPIO for cdone is not ready", dev->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->creset, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed to configure CRESET: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->cdone, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to initialize CDONE: %d", ret);
		return ret;
	}

	return 0;
}
