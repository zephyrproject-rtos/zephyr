/*
 * Copyright (c) 2026 Zhang Xingtao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT everest_es7210

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "es7210.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(es7210);

#define ES7210_REG_RESET_CONTROL  0x00
#define ES7210_REG_CLOCK_OFF      0x01
#define ES7210_REG_MAINCLK        0x02
#define ES7210_REG_POWER_DOWN     0x06
#define ES7210_REG_OSR            0x07
#define ES7210_REG_MODE_CONFIG    0x08
#define ES7210_REG_TIME_CONTROL0  0x09
#define ES7210_REG_TIME_CONTROL1  0x0A
#define ES7210_REG_SDP_INTERFACE1 0x11
#define ES7210_REG_SDP_INTERFACE2 0x12
#define ES7210_REG_ADC34_HPF2     0x20
#define ES7210_REG_ADC34_HPF1     0x21
#define ES7210_REG_ADC12_HPF2     0x22
#define ES7210_REG_ADC12_HPF1     0x23
#define ES7210_REG_ADC1_DIRECT_DB 0x1B
#define ES7210_REG_ADC2_DIRECT_DB 0x1C
#define ES7210_REG_ADC3_DIRECT_DB 0x1D
#define ES7210_REG_ADC4_DIRECT_DB 0x1E
#define ES7210_REG_CHIP_ID1       0x3D
#define ES7210_REG_CHIP_ID0       0x3E
#define ES7210_REG_CHIP_VERSION   0x3F
#define ES7210_REG_ANALOG_POWER   0x40
#define ES7210_REG_MIC12_BIAS     0x41
#define ES7210_REG_MIC34_BIAS     0x42
#define ES7210_REG_MIC1_POWER     0x47
#define ES7210_REG_MIC2_POWER     0x48
#define ES7210_REG_MIC3_POWER     0x49
#define ES7210_REG_MIC4_POWER     0x4A
#define ES7210_REG_MIC12_POWER    0x4B
#define ES7210_REG_MIC34_POWER    0x4C

struct es7210_reg_pair {
	uint8_t reg;
	uint8_t val;
};

struct es7210_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec int_gpio;
	uint8_t whoami_reg;
	uint8_t whoami_value;
	uint8_t sdp_interface1;
	uint8_t sdp_interface2;
};

struct es7210_data {
	struct k_mutex lock;
};

static const struct es7210_reg_pair es7210_init_seq[] = {
	{ES7210_REG_RESET_CONTROL, 0xFF},
	{ES7210_REG_RESET_CONTROL, 0x41},
	{ES7210_REG_CLOCK_OFF, 0x3F},
	{ES7210_REG_TIME_CONTROL0, 0x30},
	{ES7210_REG_TIME_CONTROL1, 0x30},
	{ES7210_REG_ADC12_HPF1, 0x2A},
	{ES7210_REG_ADC12_HPF2, 0x0A},
	{ES7210_REG_ADC34_HPF1, 0x2A},
	{ES7210_REG_ADC34_HPF2, 0x0A},
	{ES7210_REG_ANALOG_POWER, 0x43},
	{ES7210_REG_MIC12_BIAS, 0x70},
	{ES7210_REG_MIC34_BIAS, 0x70},
	{ES7210_REG_OSR, 0x20},
	{ES7210_REG_MAINCLK, 0xC1},
};

static int es7210_read_reg_raw(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t *val)
{
	return i2c_write_read_dt(bus, &reg, sizeof(reg), val, sizeof(*val));
}

static int es7210_write_reg_raw(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t val)
{
	uint8_t tx[2] = {reg, val};

	return i2c_write_dt(bus, tx, sizeof(tx));
}

static int es7210_read_reg_impl(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct es7210_config *cfg = dev->config;
	struct es7210_data *data = dev->data;
	int ret;

	if (val == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = es7210_read_reg_raw(&cfg->bus, reg, val);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int es7210_write_reg_impl(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct es7210_config *cfg = dev->config;
	struct es7210_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = es7210_write_reg_raw(&cfg->bus, reg, val);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int es7210_update_reg_impl(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	const struct es7210_config *cfg = dev->config;
	struct es7210_data *data = dev->data;
	uint8_t cur;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = es7210_read_reg_raw(&cfg->bus, reg, &cur);
	if (ret == 0) {
		cur = (cur & ~mask) | (val & mask);
		ret = es7210_write_reg_raw(&cfg->bus, reg, cur);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int es7210_apply_init_impl(const struct device *dev)
{
	const struct es7210_config *cfg = dev->config;
	struct es7210_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	for (size_t i = 0; i < ARRAY_SIZE(es7210_init_seq); i++) {
		ret = es7210_write_reg_raw(&cfg->bus, es7210_init_seq[i].reg, es7210_init_seq[i].val);
		if (ret != 0) {
			LOG_ERR("init write failed reg=0x%02x ret=%d", es7210_init_seq[i].reg, ret);
			break;
		}
	}

	/* Stable default: 32-bit I2S slots, non-TDM. */
	if (ret == 0) {
		ret = es7210_write_reg_raw(&cfg->bus, ES7210_REG_SDP_INTERFACE1, cfg->sdp_interface1);
	}
	if (ret == 0) {
		ret = es7210_write_reg_raw(&cfg->bus, ES7210_REG_SDP_INTERFACE2, cfg->sdp_interface2);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int es7210_set_i2s_format_impl(const struct device *dev, uint8_t sdp_interface1,
				      uint8_t sdp_interface2)
{
	int ret;

	ret = es7210_write_reg_impl(dev, ES7210_REG_SDP_INTERFACE1, sdp_interface1);
	if (ret != 0) {
		return ret;
	}

	return es7210_write_reg_impl(dev, ES7210_REG_SDP_INTERFACE2, sdp_interface2);
}

static int es7210_set_mic_gain_impl(const struct device *dev, uint8_t channel_mask,
				    uint8_t gain_regval)
{
	static const uint8_t gain_regs[] = {
		ES7210_REG_ADC1_DIRECT_DB,
		ES7210_REG_ADC2_DIRECT_DB,
		ES7210_REG_ADC3_DIRECT_DB,
		ES7210_REG_ADC4_DIRECT_DB,
	};
	int ret = 0;

	for (size_t i = 0; i < ARRAY_SIZE(gain_regs); i++) {
		if ((channel_mask & BIT(i)) == 0) {
			continue;
		}

		ret = es7210_write_reg_impl(dev, gain_regs[i], gain_regval);
		if (ret != 0) {
			break;
		}
	}

	return ret;
}

static int es7210_init(const struct device *dev)
{
	const struct es7210_config *cfg = dev->config;
	struct es7210_data *data = dev->data;
	uint8_t id1;
	uint8_t id0;
	uint8_t version = 0;
	int ret;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (cfg->int_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->int_gpio)) {
			LOG_ERR("INT GPIO not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("INT GPIO configure failed: %d", ret);
			return ret;
		}
	}

	ret = es7210_read_reg_impl(dev, cfg->whoami_reg, &id1);
	if (ret != 0) {
		LOG_WRN("WHOAMI read failed on bus=%s addr=0x%02x reg=0x%02x ret=%d",
			cfg->bus.bus->name, cfg->bus.addr, cfg->whoami_reg, ret);
		return 0;
	}

	if (id1 != cfg->whoami_value) {
		LOG_WRN("WHOAMI mismatch reg=0x%02x expected=0x%02x got=0x%02x", cfg->whoami_reg,
			cfg->whoami_value, id1);
		return 0;
	}

	ret = es7210_read_reg_impl(dev, ES7210_REG_CHIP_ID0, &id0);
	if (ret == 0) {
		(void)es7210_read_reg_impl(dev, ES7210_REG_CHIP_VERSION, &version);
		LOG_INF("ES7210 detected id1=0x%02x id0=0x%02x version=0x%02x", id1, id0, version);
	}

	return 0;
}

static const struct es7210_driver_api es7210_api = {
	.read_reg = es7210_read_reg_impl,
	.write_reg = es7210_write_reg_impl,
	.update_reg = es7210_update_reg_impl,
	.apply_init = es7210_apply_init_impl,
	.set_i2s_format = es7210_set_i2s_format_impl,
	.set_mic_gain = es7210_set_mic_gain_impl,
};

#define ES7210_CONFIG(inst)                                                                        \
	static const struct es7210_config es7210_config_##inst = {                                  \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                     \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                           \
		.whoami_reg = DT_INST_PROP_OR(inst, whoami_reg, ES7210_REG_CHIP_ID1),                 \
		.whoami_value = DT_INST_PROP_OR(inst, whoami_value, 0x72),                             \
		.sdp_interface1 = DT_INST_PROP_OR(inst, sdp_interface1, 0x80),                         \
		.sdp_interface2 = DT_INST_PROP_OR(inst, sdp_interface2, 0x00),                         \
	};                                                                                             \
	static struct es7210_data es7210_data_##inst;                                                 \
	DEVICE_DT_INST_DEFINE(inst, es7210_init, NULL, &es7210_data_##inst, &es7210_config_##inst, \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &es7210_api);

DT_INST_FOREACH_STATUS_OKAY(ES7210_CONFIG)
