/*
 * Copyright (c) 2026 NotioNext LTD.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/audio/codec.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(es7210, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define DT_DRV_COMPAT everest_es7210

/* Registers */
#define ES7210_REG_RESET               0x00
#define ES7210_REG_CLOCK_OFF           0x01
#define ES7210_REG_MAINCLK             0x02
#define ES7210_REG_MASTER_CLK          0x03
#define ES7210_REG_LRCK_DIVH           0x04
#define ES7210_REG_LRCK_DIVL           0x05
#define ES7210_REG_POWER_DOWN          0x06
#define ES7210_REG_OSR                 0x07
#define ES7210_REG_MODE_CONFIG         0x08
#define ES7210_REG_TIME_CONTROL0       0x09
#define ES7210_REG_TIME_CONTROL1       0x0A
#define ES7210_REG_SDP_INTERFACE1      0x11
#define ES7210_REG_SDP_INTERFACE2      0x12
#define ES7210_REG_ADC34_MUTERANGE     0x14
#define ES7210_REG_ADC12_MUTERANGE     0x15
#define ES7210_REG_ADC34_HPF2          0x20
#define ES7210_REG_ADC34_HPF1          0x21
#define ES7210_REG_ADC12_HPF1          0x22
#define ES7210_REG_ADC12_HPF2          0x23
#define ES7210_REG_ANALOG              0x40
#define ES7210_REG_MIC12_BIAS          0x41
#define ES7210_REG_MIC34_BIAS          0x42
#define ES7210_REG_MIC1_GAIN           0x43
#define ES7210_REG_MIC2_GAIN           0x44
#define ES7210_REG_MIC3_GAIN           0x45
#define ES7210_REG_MIC4_GAIN           0x46
#define ES7210_REG_MIC1_POWER          0x47
#define ES7210_REG_MIC2_POWER          0x48
#define ES7210_REG_MIC3_POWER          0x49
#define ES7210_REG_MIC4_POWER          0x4A
#define ES7210_REG_MIC12_POWER         0x4B
#define ES7210_REG_MIC34_POWER         0x4C

#define ES7210_SDP_FMT_MASK            0x03
#define ES7210_SDP_WORDLEN_MASK        0xE0

#define ES7210_MIC1 BIT(0)
#define ES7210_MIC2 BIT(1)
#define ES7210_MIC3 BIT(2)
#define ES7210_MIC4 BIT(3)

struct es7210_coeff_div {
	uint32_t mclk;
	uint32_t lrck;
	uint8_t adc_div;
	uint8_t dll;
	uint8_t doubler;
	uint8_t osr;
	uint8_t lrck_h;
	uint8_t lrck_l;
};

static const struct es7210_coeff_div es7210_coeffs[] = {
	/* MCLK = 256 * Fs entries commonly used by ESP32 designs */
	{ 4096000,   16000, 0x01, 0x01, 0x00, 0x20, 0x01, 0x00 },
	{ 12288000,  8000, 0x03, 0x01, 0x00, 0x20, 0x06, 0x00 },
	{ 16384000,  8000, 0x04, 0x01, 0x00, 0x20, 0x08, 0x00 },
	{ 12288000, 12000, 0x02, 0x01, 0x00, 0x20, 0x04, 0x00 },
	{ 12288000, 16000, 0x03, 0x01, 0x01, 0x20, 0x03, 0x00 },
	{ 16384000, 16000, 0x02, 0x01, 0x00, 0x20, 0x04, 0x00 },
	{ 12288000, 24000, 0x01, 0x01, 0x00, 0x20, 0x02, 0x00 },
	{ 12288000, 32000, 0x03, 0x00, 0x00, 0x20, 0x01, 0x80 },
	{ 16384000, 32000, 0x01, 0x01, 0x00, 0x20, 0x02, 0x00 },
	{ 11289600, 44100, 0x01, 0x01, 0x01, 0x20, 0x01, 0x00 },
	{ 12288000, 48000, 0x01, 0x01, 0x01, 0x20, 0x01, 0x00 },
	{ 16384000, 64000, 0x01, 0x01, 0x00, 0x20, 0x01, 0x00 },
	{ 11289600, 88200, 0x01, 0x01, 0x01, 0x20, 0x00, 0x80 },
	{ 12288000, 96000, 0x01, 0x01, 0x01, 0x20, 0x00, 0x80 },
};

struct es7210_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec power_gpio;
	uint32_t mclk_freq;
	uint8_t mic_mask;
	uint8_t mic_gain_db;
	bool use_tdm;
};

struct es7210_data {
	struct audio_codec_cfg cfg;
	bool configured;
	bool started;
	bool muted;
};

static int es7210_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct es7210_config *cfg = dev->config;
	return i2c_reg_write_byte_dt(&cfg->bus, reg, val);
}

static int es7210_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct es7210_config *cfg = dev->config;
	return i2c_reg_read_byte_dt(&cfg->bus, reg, val);
}

static int es7210_update_reg(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	int ret;
	uint8_t tmp;

	ret = es7210_read_reg(dev, reg, &tmp);
	if (ret < 0) {
		return ret;
	}

	tmp = (tmp & ~mask) | (val & mask);
	return es7210_write_reg(dev, reg, tmp);
}

static const struct es7210_coeff_div *es7210_find_coeff(uint32_t mclk, uint32_t fs)
{
	for (size_t i = 0; i < ARRAY_SIZE(es7210_coeffs); i++) {
		if (es7210_coeffs[i].mclk == mclk && es7210_coeffs[i].lrck == fs) {
			return &es7210_coeffs[i];
		}
	}

	return NULL;
}

static int es7210_hw_reset(const struct device *dev)
{
	const struct es7210_config *cfg = dev->config;
	int ret;

	if (cfg->power_gpio.port) {
		ret = gpio_pin_set_dt(&cfg->power_gpio, 1);
		if (ret < 0) {
			return ret;
		}
		k_msleep(2);
	}

	if (cfg->reset_gpio.port) {
		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}
		k_msleep(1);
		ret = gpio_pin_set_dt(&cfg->reset_gpio, 1);
		if (ret < 0) {
			return ret;
		}
		k_msleep(2);
	}

	return 0;
}

static int es7210_soft_reset(const struct device *dev)
{
	int ret;

	ret = es7210_write_reg(dev, ES7210_REG_RESET, 0xFF);
	if (ret < 0) {
		return ret;
	}

	return es7210_write_reg(dev, ES7210_REG_RESET, 0x41);
}

static int es7210_set_dai_fmt(const struct device *dev, struct audio_codec_cfg *cfg)
{
	uint8_t fmt_bits = 0;
	uint8_t wlen_bits = 0;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		return -ENOTSUP;
	}

	switch (cfg->dai_cfg.i2s.format) {
	case I2S_FMT_DATA_FORMAT_I2S:
		fmt_bits = 0x00;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		fmt_bits = 0x01;
		break;
	default:
		/* For first-pass bring-up, keep DSP/TDM behind DT use-tdm property. */
		return -ENOTSUP;
	}

	switch (cfg->dai_cfg.i2s.word_size) {
	case AUDIO_PCM_WIDTH_16_BITS:
		wlen_bits = 0x60;
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		wlen_bits = 0x00;
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		wlen_bits = 0x80;
		break;
	default:
		return -EINVAL;
	}

	return es7210_write_reg(dev, ES7210_REG_SDP_INTERFACE1, wlen_bits | fmt_bits);
}

static int es7210_set_sample_rate(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct es7210_config *dev_cfg = dev->config;
	const struct es7210_coeff_div *coeff;
	uint32_t fs = cfg->dai_cfg.i2s.frame_clk_freq;
	uint32_t mclk = cfg->mclk_freq ? cfg->mclk_freq : dev_cfg->mclk_freq;
	int ret;
	uint8_t regv;

	coeff = es7210_find_coeff(mclk, fs);
	if (coeff == NULL) {
		LOG_ERR("unsupported mclk/fs combination: %u / %u", mclk, fs);
		return -EINVAL;
	}

	regv = coeff->adc_div | (coeff->doubler << 6) | (coeff->dll << 7);
	ret = es7210_write_reg(dev, ES7210_REG_MAINCLK, regv);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_write_reg(dev, ES7210_REG_OSR, coeff->osr);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_write_reg(dev, ES7210_REG_LRCK_DIVH, coeff->lrck_h);
	if (ret < 0) {
		return ret;
	}

	return es7210_write_reg(dev, ES7210_REG_LRCK_DIVL, coeff->lrck_l);
}

/* Enable clocks and power for a MIC pair if any mic in the pair is active. */
static int es7210_enable_mic_pair(const struct device *dev, uint8_t mic_mask,
				  uint8_t pair_mask, uint8_t clk_bits,
				  uint8_t power_reg)
{
	int ret;

	if (!(mic_mask & pair_mask)) {
		return 0;
	}

	ret = es7210_update_reg(dev, ES7210_REG_CLOCK_OFF, clk_bits, 0x00);
	if (ret < 0) {
		return ret;
	}

	return es7210_write_reg(dev, power_reg, 0x00);
}

/* Set gain register for each active mic. */
static int es7210_set_mic_gains(const struct device *dev, uint8_t mic_mask,
				uint8_t gain_val)
{
	static const struct {
		uint8_t bit;
		uint8_t reg;
	} mics[] = {
		{ ES7210_MIC1, ES7210_REG_MIC1_GAIN },
		{ ES7210_MIC2, ES7210_REG_MIC2_GAIN },
		{ ES7210_MIC3, ES7210_REG_MIC3_GAIN },
		{ ES7210_MIC4, ES7210_REG_MIC4_GAIN },
	};
	int ret;

	for (int i = 0; i < ARRAY_SIZE(mics); i++) {
		if (!(mic_mask & mics[i].bit)) {
			continue;
		}
		ret = es7210_update_reg(dev, mics[i].reg, BIT(4) | 0x0F,
					BIT(4) | gain_val);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int es7210_apply_mic_selection(const struct device *dev)
{
	const struct es7210_config *cfg = dev->config;
	uint8_t gain_val = MIN(cfg->mic_gain_db / 3U, 0x0E);
	int ret;

	/* Clear SELMIC bits first */
	for (uint8_t reg = ES7210_REG_MIC1_GAIN; reg <= ES7210_REG_MIC4_GAIN; reg++) {
		ret = es7210_update_reg(dev, reg, BIT(4), 0x00);
		if (ret < 0) {
			return ret;
		}
	}

	/* Power down both pairs initially */
	ret = es7210_write_reg(dev, ES7210_REG_MIC12_POWER, 0xFF);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_MIC34_POWER, 0xFF);
	if (ret < 0) {
		return ret;
	}

	/* Enable clocks and power for active pairs */
	ret = es7210_enable_mic_pair(dev, cfg->mic_mask,
				     ES7210_MIC1 | ES7210_MIC2,
				     0x0B, ES7210_REG_MIC12_POWER);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_enable_mic_pair(dev, cfg->mic_mask,
				     ES7210_MIC3 | ES7210_MIC4,
				     0x15, ES7210_REG_MIC34_POWER);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_set_mic_gains(dev, cfg->mic_mask, gain_val);
	if (ret < 0) {
		return ret;
	}

	/* TDM only when explicitly requested. */
	return es7210_write_reg(dev, ES7210_REG_SDP_INTERFACE2,
				cfg->use_tdm ? 0x02 : 0x00);
}

static int es7210_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct es7210_data *data = dev->data;
	int ret;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		return -ENOTSUP;
	}

	/* First-pass driver expects codec as capture device, slave on bit/frame clocks. */
	if ((cfg->dai_cfg.i2s.options & I2S_OPT_BIT_CLK_SLAVE) == 0 ||
	    (cfg->dai_cfg.i2s.options & I2S_OPT_FRAME_CLK_SLAVE) == 0) {
		LOG_ERR("ES7210 driver expects codec in slave mode on BCLK/LRCK");
		return -EINVAL;
	}

	ret = es7210_soft_reset(dev);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_write_reg(dev, ES7210_REG_CLOCK_OFF, 0x3F);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_TIME_CONTROL0, 0x30);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_TIME_CONTROL1, 0x30);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_ADC12_HPF2, 0x2A);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_ADC12_HPF1, 0x0A);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_ADC34_HPF2, 0x0A);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_ADC34_HPF1, 0x2A);
	if (ret < 0) {
		return ret;
	}

	/* Slave mode */
	ret = es7210_update_reg(dev, ES7210_REG_MODE_CONFIG, 0x01, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_write_reg(dev, ES7210_REG_ANALOG, 0x43);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_MIC12_BIAS, 0x70);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_MIC34_BIAS, 0x70);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_set_sample_rate(dev, cfg);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_set_dai_fmt(dev, cfg);
	if (ret < 0) {
		return ret;
	}

	ret = es7210_apply_mic_selection(dev);
	if (ret < 0) {
		return ret;
	}

	data->cfg = *cfg;
	data->configured = true;
	return 0;
}

static int es7210_start(const struct device *dev, enum dai_dir dir)
{
	struct es7210_data *data = dev->data;
	int ret;

	if (dir != DAI_DIR_RX) {
		return -ENOTSUP;
	}
	if (!data->configured) {
		return -EACCES;
	}

	ret = es7210_write_reg(dev, ES7210_REG_CLOCK_OFF, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_POWER_DOWN, 0x00);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_ANALOG, 0x43);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_MIC1_POWER, 0x08);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_MIC2_POWER, 0x08);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_MIC3_POWER, 0x08);
	if (ret < 0) {
		return ret;
	}
	ret = es7210_write_reg(dev, ES7210_REG_MIC4_POWER, 0x08);
	if (ret < 0) {
		return ret;
	}

	data->started = true;
	return 0;
}

static int es7210_stop(const struct device *dev, enum dai_dir dir)
{
	struct es7210_data *data = dev->data;
	int ret;

	if (dir != DAI_DIR_RX) {
		return -ENOTSUP;
	}

	ret = es7210_write_reg(dev, ES7210_REG_MIC1_POWER, 0xFF);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC2_POWER, 0xFF);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC3_POWER, 0xFF);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC4_POWER, 0xFF);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC12_POWER, 0xFF);
	ret |= es7210_write_reg(dev, ES7210_REG_MIC34_POWER, 0xFF);
	ret |= es7210_write_reg(dev, ES7210_REG_ANALOG, 0xC0);
	ret |= es7210_write_reg(dev, ES7210_REG_CLOCK_OFF, 0x7F);
	ret |= es7210_write_reg(dev, ES7210_REG_POWER_DOWN, 0x07);

	data->started = false;
	return ret;
}

static int es7210_set_property(const struct device *dev, audio_property_t property,
				       audio_channel_t channel, audio_property_value_t val)
{
	ARG_UNUSED(channel);

	if (property == AUDIO_PROPERTY_INPUT_MUTE) {
		uint8_t mute_val = val.mute ? 0x03 : 0x00;

		es7210_update_reg(dev, ES7210_REG_ADC34_MUTERANGE, 0x03, mute_val);
		es7210_update_reg(dev, ES7210_REG_ADC12_MUTERANGE, 0x03, mute_val);
		return 0;
	}

	return -ENOTSUP;
}

static int es7210_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static void es7210_start_output_wrapper(const struct device *dev)
{
	es7210_start(dev, DAI_DIR_RX);
}

static void es7210_stop_output_wrapper(const struct device *dev)
{
	es7210_stop(dev, DAI_DIR_RX);
}

static int es7210_init(const struct device *dev)
{
	const struct es7210_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (cfg->reset_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->power_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->power_gpio)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->power_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	ret = es7210_hw_reset(dev);
	if (ret < 0) {
		return ret;
	}

	return es7210_soft_reset(dev);
}

static const struct audio_codec_api es7210_api = {
	.configure = es7210_configure,
	.start_output = (void (*)(const struct device *))es7210_start_output_wrapper,
	.stop_output = (void (*)(const struct device *))es7210_stop_output_wrapper,
	.set_property = es7210_set_property,
	.apply_properties = es7210_apply_properties,
};

#define ES7210_INIT(inst)                                                        \
	static const struct es7210_config es7210_cfg_##inst = {                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                  \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),     \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, power_gpios, {0}),     \
		.mclk_freq = DT_INST_PROP_OR(inst, mclk_freq, 4096000),             \
		.mic_mask = DT_INST_PROP_OR(inst, mic_mask, 0x03),                  \
		.mic_gain_db = DT_INST_PROP_OR(inst, mic_gain_db, 24),              \
		.use_tdm = DT_INST_PROP(inst, use_tdm),                             \
	};                                                                      \
	static struct es7210_data es7210_data_##inst;                           \
	DEVICE_DT_INST_DEFINE(inst, es7210_init, NULL,                          \
			      &es7210_data_##inst, &es7210_cfg_##inst,            \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY,      \
			      &es7210_api);

DT_INST_FOREACH_STATUS_OKAY(ES7210_INIT)
