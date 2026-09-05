/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#include "tlv320aic3104_bus.h"
#include "tlv320aic3104_priv.h"
#include "tlv320aic3104_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tlv320aic3104);

bool tlv320aic3104_bus_is_ready(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;

	return i2c_is_ready_dt(&cfg->bus);
}

int tlv320aic3104_bus_write_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t val)
{
	struct tlv320aic3104_data *data = dev->data;
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	if (data->page_cache != page) {
		ret = i2c_reg_write_byte_dt(&cfg->bus, PAGE_CONTROL_ADDR, page);
		if (ret < 0) {
			LOG_ERR("Failed to set page %u: %d", page, ret);
			return ret;
		}
		data->page_cache = page;
	}

	ret = i2c_reg_write_byte_dt(&cfg->bus, addr, val);
	if (ret < 0) {
		LOG_ERR("Failed to write reg 0x%02x: %d", addr, ret);
		return ret;
	}

	return 0;
}

int tlv320aic3104_bus_read_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t *val)
{
	struct tlv320aic3104_data *data = dev->data;
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	if (data->page_cache != page) {
		ret = i2c_reg_write_byte_dt(&cfg->bus, PAGE_CONTROL_ADDR, page);
		if (ret < 0) {
			LOG_ERR("Failed to set page %u: %d", page, ret);
			return ret;
		}
		data->page_cache = page;
	}

	ret = i2c_reg_read_byte_dt(&cfg->bus, addr, val);
	if (ret < 0) {
		LOG_ERR("Failed to read reg 0x%02x: %d", addr, ret);
		return ret;
	}

	return 0;
}
