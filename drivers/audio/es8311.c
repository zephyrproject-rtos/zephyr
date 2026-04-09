/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT everest_es8311

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/audio/es8311.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(es8311, CONFIG_AUDIO_CODEC_LOG_LEVEL);

/* ES8311 Register Definitions */
#define ES8311_REG00_RESET          0x00
#define ES8311_REG01_CLK_MANAGER    0x01
#define ES8311_REG02_CLK_MANAGER    0x02
#define ES8311_REG03_CLK_MANAGER    0x03
#define ES8311_REG04_CLK_MANAGER    0x04
#define ES8311_REG05_CLK_MANAGER    0x05
#define ES8311_REG06_CLK_MANAGER    0x06
#define ES8311_REG07_CLK_MANAGER    0x07
#define ES8311_REG08_CLK_MANAGER    0x08
#define ES8311_REG0D_SDP_IN         0x0D
#define ES8311_REG0E_SDP_OUT        0x0E
#define ES8311_REG0E_SYSTEM         0x0E
#define ES8311_REG10_SYSTEM         0x10
#define ES8311_REG11_SYSTEM         0x11
#define ES8311_REG12_SYSTEM         0x12
#define ES8311_REG13_SYSTEM         0x13
#define ES8311_REG14_SYSTEM         0x14
#define ES8311_REG15_ADC            0x15
#define ES8311_REG16_ADC            0x16
#define ES8311_REG17_ADC            0x17
#define ES8311_REG1B_ADC            0x1B
#define ES8311_REG1C_ADC            0x1C
#define ES8311_REG31_DAC            0x31
#define ES8311_REG32_DAC            0x32
#define ES8311_REG37_DAC            0x37
#define ES8311_REG44_GPIO           0x44
#define ES8311_REG45_GP             0x45
#define ES8311_REGFD_CHD1           0xFD
#define ES8311_REGFE_CHD2           0xFE
#define ES8311_REGFF_CHVER          0xFF

/* Clock coefficient structure for different sample rates */
struct es8311_coeff_div {
	uint32_t mclk;
	uint32_t rate;
	uint8_t pre_div;
	uint8_t pre_multi;
	uint8_t adc_div;
	uint8_t dac_div;
	uint8_t fs_mode;
	uint8_t lrck_h;
	uint8_t lrck_l;
	uint8_t bclk_div;
	uint8_t adc_osr;
	uint8_t dac_osr;
};

/* Clock coefficients for common sample rates */
static const struct es8311_coeff_div coeff_div[] = {
	/* 8kHz */
	{12288000, 8000,  0x06, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
	{6144000,  8000,  0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
	
	/* 11.025kHz */
	{11289600, 11025, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
	{5644800,  11025, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
	
	/* 12kHz */
	{12288000, 12000, 0x04, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
	{6144000,  12000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
	
	/* 16kHz */
	{12288000, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
	{6144000,  16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x20},
	
	/* 22.05kHz */
	{11289600, 22050, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	{5644800,  22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	
	/* 24kHz */
	{12288000, 24000, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	{6144000,  24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	
	/* 32kHz */
	{12288000, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	{6144000,  32000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	
	/* 44.1kHz */
	{11289600, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	{5644800,  44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	
	/* 48kHz */
	{12288000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	{6144000,  48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	
	/* 64kHz */
	{12288000, 64000, 0x03, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	{6144000,  64000, 0x01, 0x04, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
	
	/* 88.2kHz */
	{11289600, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	{5644800,  88200, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	
	/* 96kHz */
	{12288000, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
	{6144000,  96000, 0x01, 0x04, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
};

struct es8311_data {
	struct es8311_config config;
	bool initialized;
	bool enabled;
};

struct es8311_config_dt {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec pa_gpio;
};

static int es8311_write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct es8311_config_dt *cfg = dev->config;
	uint8_t data[2] = {reg, value};
	
	return i2c_write_dt(&cfg->i2c, data, sizeof(data));
}

static int es8311_read_reg(const struct device *dev, uint8_t reg, uint8_t *value)
{
	const struct es8311_config_dt *cfg = dev->config;
	
	return i2c_write_read_dt(&cfg->i2c, &reg, 1, value, 1);
}

static int es8311_get_coeff(uint32_t mclk, uint32_t rate)
{
	for (int i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk) {
			return i;
		}
	}
	return -ENOTSUP;
}

static int es8311_configure_clocks(const struct device *dev, const struct es8311_config *config)
{
	int coeff_idx;
	uint8_t reg_val;
	uint32_t mclk = 6144000; /* Default MCLK for ESP32-S3-BOX-3 */
	
	/* Find appropriate clock coefficients */
	coeff_idx = es8311_get_coeff(mclk, config->sample_rate);
	if (coeff_idx < 0) {
		LOG_ERR("Unsupported sample rate: %d", config->sample_rate);
		return coeff_idx;
	}
	
	const struct es8311_coeff_div *coeff = &coeff_div[coeff_idx];
	
	/* Configure clock dividers */
	es8311_read_reg(dev, ES8311_REG02_CLK_MANAGER, &reg_val);
	reg_val &= 0x07;
	reg_val |= (coeff->pre_div - 1) << 5;
	reg_val |= (coeff->pre_multi == 2 ? 1 : 0) << 3;
	es8311_write_reg(dev, ES8311_REG02_CLK_MANAGER, reg_val);
	
	/* ADC and DAC dividers */
	reg_val = 0x00;
	reg_val |= (coeff->adc_div - 1) << 4;
	reg_val |= (coeff->dac_div - 1) << 0;
	es8311_write_reg(dev, ES8311_REG05_CLK_MANAGER, reg_val);
	
	/* FS mode and OSR */
	es8311_read_reg(dev, ES8311_REG03_CLK_MANAGER, &reg_val);
	reg_val &= 0x80;
	reg_val |= coeff->fs_mode << 6;
	reg_val |= coeff->adc_osr << 0;
	es8311_write_reg(dev, ES8311_REG03_CLK_MANAGER, reg_val);
	
	es8311_read_reg(dev, ES8311_REG04_CLK_MANAGER, &reg_val);
	reg_val &= 0x80;
	reg_val |= coeff->dac_osr << 0;
	es8311_write_reg(dev, ES8311_REG04_CLK_MANAGER, reg_val);
	
	/* LRCK dividers */
	es8311_read_reg(dev, ES8311_REG07_CLK_MANAGER, &reg_val);
	reg_val &= 0xC0;
	reg_val |= coeff->lrck_h << 0;
	es8311_write_reg(dev, ES8311_REG07_CLK_MANAGER, reg_val);
	
	es8311_write_reg(dev, ES8311_REG08_CLK_MANAGER, coeff->lrck_l);
	
	/* BCLK divider */
	es8311_read_reg(dev, ES8311_REG06_CLK_MANAGER, &reg_val);
	reg_val &= 0xE0;
	reg_val |= (coeff->bclk_div - 1) << 0;
	es8311_write_reg(dev, ES8311_REG06_CLK_MANAGER, reg_val);
	
	return 0;
}

int es8311_initialize(const struct device *dev, const struct es8311_config *config)
{
	struct es8311_data *data = dev->data;
	const struct es8311_config_dt *cfg = dev->config;
	uint8_t reg_val;
	int ret;
	
	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
	
	/* Reset codec if reset GPIO is available */
	if (cfg->reset_gpio.port) {
		gpio_pin_set_dt(&cfg->reset_gpio, 1);
		k_sleep(K_MSEC(10));
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_sleep(K_MSEC(10));
	}
	
	/* Enable PA if PA GPIO is available */
	if (cfg->pa_gpio.port) {
		gpio_pin_set_dt(&cfg->pa_gpio, 1);
		k_sleep(K_MSEC(10));
	}
	
	/* Give codec time to power up */
	k_sleep(K_MSEC(100));
	
	/* Verify codec presence */
	ret = es8311_read_reg(dev, ES8311_REGFD_CHD1, &reg_val);
	if (ret < 0) {
		LOG_ERR("Failed to read ES8311 chip ID");
		return ret;
	}
	
	LOG_INF("ES8311 chip ID: 0x%02x", reg_val);
	
	/* Reset ES8311 */
	es8311_write_reg(dev, ES8311_REG00_RESET, 0x1F);
	k_sleep(K_MSEC(20));
	es8311_write_reg(dev, ES8311_REG00_RESET, 0x00);
	k_sleep(K_MSEC(20));
	
	/* Initial clock and system setup */
	es8311_write_reg(dev, ES8311_REG01_CLK_MANAGER, 0x30);
	es8311_write_reg(dev, ES8311_REG02_CLK_MANAGER, 0x00);
	es8311_write_reg(dev, ES8311_REG03_CLK_MANAGER, 0x10);
	es8311_write_reg(dev, ES8311_REG16_ADC, 0x24);
	es8311_write_reg(dev, ES8311_REG04_CLK_MANAGER, 0x10);
	es8311_write_reg(dev, ES8311_REG05_CLK_MANAGER, 0x00);
	es8311_write_reg(dev, ES8311_REG10_SYSTEM, 0x1F);
	es8311_write_reg(dev, ES8311_REG11_SYSTEM, 0x7F);
	es8311_write_reg(dev, ES8311_REG00_RESET, 0x80);
	
	/* Configure for slave mode */
	es8311_read_reg(dev, ES8311_REG00_RESET, &reg_val);
	if (config->master_mode) {
		reg_val |= 0x40;
	} else {
		reg_val &= 0xBF;
	}
	es8311_write_reg(dev, ES8311_REG00_RESET, reg_val);
	
	/* Clock source configuration */
	reg_val = 0x3F;
	if (config->use_mclk) {
		reg_val &= 0x7F;
	} else {
		reg_val |= 0x80;
	}
	if (config->invert_mclk) {
		reg_val |= 0x40;
	} else {
		reg_val &= ~0x40;
	}
	es8311_write_reg(dev, ES8311_REG01_CLK_MANAGER, reg_val);
	
	/* SCLK configuration */
	es8311_read_reg(dev, ES8311_REG06_CLK_MANAGER, &reg_val);
	if (config->invert_sclk) {
		reg_val |= 0x20;
	} else {
		reg_val &= ~0x20;
	}
	es8311_write_reg(dev, ES8311_REG06_CLK_MANAGER, reg_val);
	
	/* System configuration */
	es8311_write_reg(dev, ES8311_REG13_SYSTEM, 0x10);
	es8311_write_reg(dev, ES8311_REG1B_ADC, 0x0A);
	es8311_write_reg(dev, ES8311_REG1C_ADC, 0x6A);
	es8311_write_reg(dev, ES8311_REG44_GPIO, 0x50);
	
	/* Configure clocks for sample rate */
	ret = es8311_configure_clocks(dev, config);
	if (ret < 0) {
		return ret;
	}
	
	/* Configure I2S format and bit depth */
	uint8_t format_bits = 0x0C; /* Default 16-bit I2S */
	switch (config->bit_depth) {
	case ES8311_BIT_DEPTH_16:
		format_bits = 0x0C;
		break;
	case ES8311_BIT_DEPTH_24:
		format_bits = 0x00;
		break;
	case ES8311_BIT_DEPTH_32:
		format_bits = 0x10;
		break;
	}
	
	switch (config->format) {
	case ES8311_FORMAT_I2S:
		/* Keep default */
		break;
	case ES8311_FORMAT_LEFT_JUSTIFIED:
		format_bits |= 0x01;
		break;
	case ES8311_FORMAT_DSP_A:
		format_bits |= 0x03;
		break;
	}
	
	/* Enable DAC path, disable ADC path for playback */
	uint8_t dac_iface = format_bits & ~0x40;
	uint8_t adc_iface = format_bits | 0x40;
	
	es8311_write_reg(dev, ES8311_REG0D_SDP_IN, dac_iface);
	es8311_write_reg(dev, ES8311_REG0E_SDP_OUT, adc_iface);
	
	/* Power up and enable */
	es8311_write_reg(dev, ES8311_REG17_ADC, 0xBF);
	es8311_write_reg(dev, ES8311_REG0E_SYSTEM, 0x02);
	es8311_write_reg(dev, ES8311_REG12_SYSTEM, 0x00);
	es8311_write_reg(dev, ES8311_REG14_SYSTEM, 0x1A);
	es8311_write_reg(dev, ES8311_REG15_ADC, 0x40);
	es8311_write_reg(dev, ES8311_REG37_DAC, 0x08);
	es8311_write_reg(dev, ES8311_REG45_GP, 0x00);
	
	/* Configure DAC for maximum volume and unmute */
	es8311_write_reg(dev, ES8311_REG31_DAC, 0x00);
	es8311_write_reg(dev, ES8311_REG32_DAC, 0xFF);
	
	k_sleep(K_MSEC(100));
	
	/* Store configuration */
	data->config = *config;
	data->initialized = true;
	data->enabled = true;
	
	LOG_INF("ES8311 initialized successfully");
	return 0;
}

int es8311_set_volume(const struct device *dev, uint8_t volume)
{
	struct es8311_data *data = dev->data;
	
	if (!data->initialized) {
		return -ENODEV;
	}
	
	return es8311_write_reg(dev, ES8311_REG32_DAC, volume);
}

int es8311_set_mute(const struct device *dev, bool mute)
{
	struct es8311_data *data = dev->data;
	uint8_t reg_val;
	int ret;
	
	if (!data->initialized) {
		return -ENODEV;
	}
	
	ret = es8311_read_reg(dev, ES8311_REG31_DAC, &reg_val);
	if (ret < 0) {
		return ret;
	}
	
	reg_val &= 0x9F;
	if (mute) {
		reg_val |= 0x60;
	}
	
	return es8311_write_reg(dev, ES8311_REG31_DAC, reg_val);
}

int es8311_enable(const struct device *dev, bool enable)
{
	struct es8311_data *data = dev->data;
	const struct es8311_config_dt *cfg = dev->config;
	
	if (!data->initialized) {
		return -ENODEV;
	}
	
	if (enable == data->enabled) {
		return 0;
	}
	
	if (enable) {
		/* Enable PA if available */
		if (cfg->pa_gpio.port) {
			gpio_pin_set_dt(&cfg->pa_gpio, 1);
		}
		/* Power up sequence would go here */
	} else {
		/* Disable PA if available */
		if (cfg->pa_gpio.port) {
			gpio_pin_set_dt(&cfg->pa_gpio, 0);
		}
		/* Power down sequence would go here */
	}
	
	data->enabled = enable;
	return 0;
}

static int es8311_init(const struct device *dev)
{
	const struct es8311_config_dt *cfg = dev->config;
	int ret;
	
	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}
	
	/* Configure reset GPIO if available */
	if (cfg->reset_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("Reset GPIO not ready");
			return -ENODEV;
		}
		
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO: %d", ret);
			return ret;
		}
	}
	
	/* Configure PA GPIO if available */
	if (cfg->pa_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->pa_gpio)) {
			LOG_ERR("PA GPIO not ready");
			return -ENODEV;
		}
		
		ret = gpio_pin_configure_dt(&cfg->pa_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure PA GPIO: %d", ret);
			return ret;
		}
	}
	
	LOG_INF("ES8311 driver initialized");
	return 0;
}

#define ES8311_DEFINE(inst)								\
	static struct es8311_data es8311_data_##inst;					\
										\
	static const struct es8311_config_dt es8311_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),	\
		.pa_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, pa_gpios, {0}),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, es8311_init, NULL,					\
			      &es8311_data_##inst, &es8311_config_##inst,		\
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY,		\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(ES8311_DEFINE)
