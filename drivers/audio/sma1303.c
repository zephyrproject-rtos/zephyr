/*
 * Copyright 2025 Iron Device Corporation.
 * Copyright (C) 2025 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT iron_sma1303


#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#include "sma1303.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(irondevice_sma1303, CONFIG_AUDIO_CODEC_LOG_LEVEL);

union sma1303_bus {
	struct i2c_dt_spec i2c;
};

#define PLL_MATCH(_input_clk_name, _output_clk_name, _input_clk,\
		_post_n, _n, _vco,  _p_cp)\
{\
	.input_clk_name		= _input_clk_name,\
	.output_clk_name	= _output_clk_name,\
	.input_clk		= _input_clk,\
	.post_n			= _post_n,\
	.n			= _n,\
	.vco			= _vco,\
	.p_cp			=_p_cp,\
}

/* PLL clock setting Table */
struct sma1303_pll_match {
	char *input_clk_name;
	char *output_clk_name;
	unsigned int input_clk;
	unsigned int post_n;
	unsigned int n;
	unsigned int vco;
	unsigned int p_cp;
};

static struct sma1303_pll_match sma1303_pll_matches[] = {
/* in_clk_name, out_clk_name, input_clk post_n, n, vco, p_cp */
PLL_MATCH("1.411MHz",  "24.595MHz", 1411200,  0x07, 0xF4, 0x8B, 0x03),
PLL_MATCH("1.536MHz",  "24.576MHz", 1536000,  0x07, 0xE0, 0x8B, 0x03),
PLL_MATCH("2.000MHz",  "24.571MHz", 2000000,  0x07, 0xAC, 0x8B, 0x03),
PLL_MATCH("3.072MHz",  "24.576MHz", 3072000,  0x07, 0x70, 0x8B, 0x03),
PLL_MATCH("6.144MHz",  "24.576MHz", 6144000,  0x07, 0x70, 0x8B, 0x07),
PLL_MATCH("12.288MHz", "24.576MHz", 12288000, 0x07, 0x70, 0x8B, 0x0B),
PLL_MATCH("19.2MHz",   "24.343MHz", 19200000, 0x07, 0x47, 0x8B, 0x0A),
PLL_MATCH("24.576MHz", "24.576MHz", 24576000, 0x07, 0x70, 0x8B, 0x0F),
};

static const struct reg_default sma1303_reg_def[] = {
	{ 0x00, 0x80 },
	{ 0x01, 0x00 },
	{ 0x02, 0x00 },
	{ 0x03, 0x11 },
	{ 0x04, 0x17 },
	{ 0x09, 0x00 },
	{ 0x0A, 0x31 },
	{ 0x0B, 0x98 },
	{ 0x0C, 0x84 },
	{ 0x0D, 0x07 },
	{ 0x0E, 0x3F },
	{ 0x10, 0x00 },
	{ 0x11, 0x00 },
	{ 0x12, 0x00 },
	{ 0x14, 0x5C },
	{ 0x15, 0x01 },
	{ 0x16, 0x0F },
	{ 0x17, 0x0F },
	{ 0x18, 0x0F },
	{ 0x19, 0x00 },
	{ 0x1A, 0x00 },
	{ 0x1B, 0x00 },
	{ 0x23, 0x19 },
	{ 0x24, 0x00 },
	{ 0x25, 0x00 },
	{ 0x26, 0x04 },
	{ 0x33, 0x00 },
	{ 0x36, 0x92 },
	{ 0x37, 0x27 },
	{ 0x3B, 0x5A },
	{ 0x3C, 0x20 },
	{ 0x3D, 0x00 },
	{ 0x3E, 0x03 },
	{ 0x3F, 0x0C },
	{ 0x8B, 0x07 },
	{ 0x8C, 0x70 },
	{ 0x8D, 0x8B },
	{ 0x8E, 0x6F },
	{ 0x8F, 0x03 },
	{ 0x90, 0x26 },
	{ 0x91, 0x42 },
	{ 0x92, 0xE0 },
	{ 0x94, 0x35 },
	{ 0x95, 0x0C },
	{ 0x96, 0x42 },
	{ 0x97, 0x95 },
	{ 0xA0, 0x00 },
	{ 0xA1, 0x3B },
	{ 0xA2, 0xC8 },
	{ 0xA3, 0x28 },
	{ 0xA4, 0x40 },
	{ 0xA5, 0x01 },
	{ 0xA6, 0x41 },
	{ 0xA7, 0x00 },
};

typedef bool (*sma1303_bus_is_ready_fn)(const union sma1303_bus *bus);

struct sma1303_driver_config {
	const union sma1303_bus bus;
	sma1303_bus_is_ready_fn bus_is_ready;
};

static bool sma1303_bus_is_ready_i2c(const union sma1303_bus *bus)
{
	return device_is_ready(bus->i2c.bus);
}

static void sma1303_reg_read(const struct device *dev, uint8_t addr, uint8_t *val)
{
	const struct sma1303_driver_config *config = dev->config;

	int count = 0;
	do {
		int ret = i2c_reg_read_byte_dt(&config->bus.i2c, addr, val);
		if (!ret) {
			break;
		} else {
			count++;
		}
	} while (count < 10);
	if (count) {
		LOG_ERR("retry r 0x%02x: %d", addr, count);
	}
}

static void sma1303_reg_write(const struct device *dev, uint8_t addr, uint8_t val)
{
	const struct sma1303_driver_config *config = dev->config;

	int count = 0;
	do {
		int ret = i2c_reg_write_byte_dt(&config->bus.i2c, addr, val);
		if (!ret) {
			break;
		} else {
			count++;
		}
	} while (count < 10);
	if (count) {
		LOG_ERR("retry w 0x%02x: %d", addr, count);
	}
}

static void sma1303_reg_update(const struct device *dev, uint8_t addr, uint8_t mask,
			      uint8_t val)
{
	uint8_t orig;

	sma1303_reg_read(dev, addr, &orig);
	val = (orig & ~mask) | (val & mask);

	sma1303_reg_write(dev, addr, val);
}

static int sma1303_set_pcm_volume(const struct device *dev, int vol)
{
	sma1303_reg_write(dev, SMA1303_0A_SPK_VOL, vol);

	return 0;
}

static int sma1303_set_mute(const struct device *dev, const bool mute)
{

	if (mute) {
		sma1303_reg_update(dev, SMA1303_0E_MUTE_VOL_CTRL,
					SMA1303_SPK_MUTE_MASK,
					SMA1303_SPK_MUTE);
	}

	return 0;
}

static int sma1303_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return sma1303_set_mute(dev, val.mute);
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return sma1303_set_pcm_volume(dev, val.vol);
	default:
		return -EINVAL;
	}
}

static int sma1303_global_en_event(const struct device *dev, const bool enable)
{
	if (enable) {
		sma1303_reg_update(dev, SMA1303_8E_PLL_CTRL,
					SMA1303_PLL_PD2_MASK,
					SMA1303_PLL_OPERATION2);

		sma1303_reg_update(dev, SMA1303_00_SYSTEM_CTRL,
					SMA1303_POWER_MASK,
					SMA1303_POWER_ON);

		sma1303_reg_update(dev, SMA1303_10_SYSTEM_CTRL1,
					SMA1303_SPK_MODE_MASK,
					SMA1303_SPK_STEREO);

		sma1303_reg_update(dev, SMA1303_0E_MUTE_VOL_CTRL,
					SMA1303_SPK_MUTE_MASK,
					SMA1303_SPK_UNMUTE);
	} else {

		sma1303_reg_update(dev, SMA1303_0E_MUTE_VOL_CTRL,
					SMA1303_SPK_MUTE_MASK,
					SMA1303_SPK_MUTE);

		k_msleep(55); // prevent unintended sounds

		sma1303_reg_update(dev, SMA1303_10_SYSTEM_CTRL1,
					SMA1303_SPK_MODE_MASK,
					SMA1303_SPK_OFF);

		sma1303_reg_update(dev, SMA1303_00_SYSTEM_CTRL,
					SMA1303_POWER_MASK,
					SMA1303_POWER_OFF);

		sma1303_reg_update(dev, SMA1303_8E_PLL_CTRL,
					SMA1303_PLL_PD2_MASK,
					SMA1303_PLL_PD2);
	}

	return 0;
}

static void sma1303_stop_output(const struct device *dev)
{
	sma1303_global_en_event(dev, false);
}

static void sma1303_start_output(const struct device *dev)
{
	sma1303_global_en_event(dev, true);
}

static int sma1303_set_pll(const struct device *dev, const uint32_t freq)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sma1303_pll_matches); i++) {
			if (sma1303_pll_matches[i].input_clk >= freq)
				break;
	}
	if (i == ARRAY_SIZE(sma1303_pll_matches)) {
		LOG_ERR("No matching value between pll table and SCK");
		return -EINVAL;
	}

	LOG_INF("sma1303_set_pll bclk=%d", sma1303_pll_matches[i].input_clk);

	sma1303_reg_update(dev, SMA1303_A2_TOP_MAN1,
				SMA1303_PLL_PD_MASK | SMA1303_PLL_REF_CLK_MASK,
				SMA1303_PLL_OPERATION | SMA1303_PLL_SCK);

	sma1303_reg_write(dev, SMA1303_8B_PLL_POST_N,
			sma1303_pll_matches[i].post_n);
	sma1303_reg_write(dev, SMA1303_8C_PLL_N,
			sma1303_pll_matches[i].n);
	sma1303_reg_write(dev, SMA1303_8D_PLL_A_SETTING,
			sma1303_pll_matches[i].vco);
	sma1303_reg_write(dev, SMA1303_8F_PLL_P_CP,
			sma1303_pll_matches[i].p_cp);

	return 0;
}

static int sma1303_set_frame_clk_freq(const struct device *dev, const uint32_t freq)
{
	switch (freq) {
	case AUDIO_PCM_RATE_8K:
	case AUDIO_PCM_RATE_11P025K:
	case AUDIO_PCM_RATE_16K:
	case AUDIO_PCM_RATE_22P05K:
	case AUDIO_PCM_RATE_24K:
	case AUDIO_PCM_RATE_32K:
	case AUDIO_PCM_RATE_44P1K:
	case AUDIO_PCM_RATE_48K:
	case AUDIO_PCM_RATE_96K:
		sma1303_reg_update(dev, SMA1303_A2_TOP_MAN1,
					SMA1303_DAC_DN_CONV_MASK,
					SMA1303_DAC_DN_CONV_DISABLE);
		sma1303_reg_update(dev, SMA1303_01_INPUT1_CTRL1,
					SMA1303_LEFTPOL_MASK,
					SMA1303_LOW_FIRST_CH);
		break;
	case AUDIO_PCM_RATE_192K:
		sma1303_reg_update(dev, SMA1303_A2_TOP_MAN1,
					SMA1303_DAC_DN_CONV_MASK,
					SMA1303_DAC_DN_CONV_ENABLE);
		sma1303_reg_update(dev, SMA1303_01_INPUT1_CTRL1,
					SMA1303_LEFTPOL_MASK,
					SMA1303_HIGH_FIRST_CH);
		break;
	default:
		LOG_ERR("Unsupported frame clock frequency: %d Hz", freq);
		return -EINVAL;
	}

	return 0;
}

static int sma1303_set_word_size(const struct device *dev, const uint8_t word_size)
{
	switch (word_size){
	case AUDIO_PCM_WIDTH_16_BITS:
		sma1303_reg_update(dev, SMA1303_A4_TOP_MAN3,
					SMA1303_SCK_RATE_MASK,
					SMA1303_SCK_64FS);
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		sma1303_reg_update(dev, SMA1303_A4_TOP_MAN3,
					SMA1303_SCK_RATE_MASK,
					SMA1303_SCK_64FS);
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		sma1303_reg_update(dev, SMA1303_A4_TOP_MAN3,
					SMA1303_SCK_RATE_MASK,
					SMA1303_SCK_64FS);
		break;
	case AUDIO_PCM_WIDTH_20_BITS:
	default:
		LOG_ERR("Unsupported bit widths: %d bits", word_size);
		return -EINVAL;
	}

	return 0;
}

static int sma1303_set_format(const struct device *dev, const i2s_fmt_t i2s_fmt)
{
	switch (FIELD_GET(I2S_FMT_DATA_FORMAT_MASK, i2s_fmt)) {
	case I2S_FMT_DATA_FORMAT_I2S:
		sma1303_reg_update(dev, SMA1303_01_INPUT1_CTRL1,
					SMA1303_I2S_MODE_MASK,
					SMA1303_STANDARD_I2S);
		sma1303_reg_update(dev, SMA1303_A4_TOP_MAN3,
					SMA1303_O_FORMAT_MASK,
					SMA1303_O_FMT_I2S);
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
	default:
		LOG_ERR("In the current version, data formats other than I2S are not supported");
		return -EINVAL;
	}

	switch (FIELD_GET(I2S_FMT_CLK_FORMAT_MASK, i2s_fmt)) {
	case I2S_FMT_CLK_NF_NB:
		sma1303_reg_update(dev, SMA1303_01_INPUT1_CTRL1,
					SMA1303_LEFTPOL_MASK | SMA1303_SCK_RISING_MASK,
					SMA1303_LOW_FIRST_CH | SMA1303_SCK_RISING_EDGE);
		break;
	case I2S_FMT_CLK_NF_IB:
		sma1303_reg_update(dev, SMA1303_01_INPUT1_CTRL1,
					SMA1303_LEFTPOL_MASK | SMA1303_SCK_RISING_MASK,
					SMA1303_LOW_FIRST_CH | SMA1303_SCK_FALLING_EDGE);
		break;
	case I2S_FMT_CLK_IF_NB:
		sma1303_reg_update(dev, SMA1303_01_INPUT1_CTRL1,
					SMA1303_LEFTPOL_MASK | SMA1303_SCK_RISING_MASK,
					SMA1303_HIGH_FIRST_CH | SMA1303_SCK_FALLING_EDGE);
		break;
	case I2S_FMT_CLK_IF_IB:
		sma1303_reg_update(dev, SMA1303_01_INPUT1_CTRL1,
					SMA1303_LEFTPOL_MASK | SMA1303_SCK_RISING_MASK,
					SMA1303_HIGH_FIRST_CH | SMA1303_SCK_RISING_EDGE);
		break;
	default:
		LOG_ERR("Invalid DAI clock polarity");
		return -EINVAL;
	}

	return 0;
}

static int sma1303_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	uint32_t bclk_freq;
	int ret;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		// AUDIO_DAI_TYPE_LEFT_JUSTIFIED
		// AUDIO_DAI_TYPE_RIGHT_JUSTIFIED
		// AUDIO_DAI_TYPE_PCMA
		// AUDIO_DAI_TYPE_PCMB
		// AUDIO_DAI_TYPE_INVALID
		LOG_ERR("The driver currently supports only I2S in this version");
		return -EINVAL;
	}

	if (cfg->dai_route != AUDIO_ROUTE_PLAYBACK) {
		// AUDIO_ROUTE_BYPASS
		// AUDIO_ROUTE_PLAYBACK_CAPTURE
		// AUDIO_ROUTE_CAPTURE
		LOG_ERR("The driver currently supports only PLAYBACK mode in this version");
		return -EINVAL;
	}

	if (cfg->dai_cfg.i2s.channels != 2) {
		LOG_ERR("The driver currently supports only 2 channels in this version");
		return -EINVAL;
	}

	LOG_INF("sma1303_configure freq=%d", cfg->dai_cfg.i2s.frame_clk_freq);
	ret = sma1303_set_frame_clk_freq(dev, cfg->dai_cfg.i2s.frame_clk_freq);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("sma1303_configure word_size=%d", cfg->dai_cfg.i2s.word_size);
	ret = sma1303_set_word_size(dev, cfg->dai_cfg.i2s.word_size);
	if (ret < 0) {
		return ret;
	}
	LOG_INF("sma1303_configure format=%d", cfg->dai_cfg.i2s.format);
	ret = sma1303_set_format(dev, cfg->dai_cfg.i2s.format);
	if (ret < 0) {
		return ret;
	}

	bclk_freq = cfg->dai_cfg.i2s.frame_clk_freq
			* 32 * cfg->dai_cfg.i2s.channels;
	LOG_INF("bclk_freq=%d", bclk_freq);
	return sma1303_set_pll(dev, bclk_freq);
}

static int sma1303_apply_setting(const struct device *dev)
{

	for (int i = 0; i < ARRAY_SIZE(sma1303_reg_def); i++) {
		sma1303_reg_write(dev, sma1303_reg_def[i].reg, sma1303_reg_def[i].def);
	}

	return 0;
}

static int sma1303_hw_init(const struct device *dev)
{
	uint8_t val = 0, ver = 0;
	int ret, i = 5;

	while (!(val & SMA1303_FF_DEVICE_INDEX)) {
		k_usleep(1000);
		sma1303_reg_read(dev, SMA1303_FF_DEVICE_INDEX, &val);
		i--;
		if (i < 0) {
			return -ETIMEDOUT;
		}
	}

	ver = val & 0x07;
	LOG_INF("Found Device(SMA1303) is MVT%d", ver);

	sma1303_reg_update(dev, SMA1303_00_SYSTEM_CTRL,
				SMA1303_RESETBYI2C_MASK,
				SMA1303_RESETBYI2C_RESET);

	ret = sma1303_apply_setting(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int sma1303_init(const struct device *dev)
{
	const struct sma1303_driver_config *config = dev->config;

	if (!config->bus_is_ready) {
		return -ENODEV;
	}

	return sma1303_hw_init(dev);
}

static const struct audio_codec_api sma1303_driver_api = {
	.configure = sma1303_configure,
	.start_output = sma1303_start_output,
	.stop_output = sma1303_stop_output,
	.set_property = sma1303_set_property,
};


#define SMA1303_INIT(n)                                                               	            \
	static const struct sma1303_driver_config sma1303_device_config_##n = {                     \
		.bus = {.i2c = I2C_DT_SPEC_INST_GET(n)},                                            \
		.bus_is_ready = sma1303_bus_is_ready_i2c};                                          \
                                                                                                    \
	DEVICE_DT_INST_DEFINE(n, sma1303_init, NULL, NULL, &sma1303_device_config_##n,              \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &sma1303_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SMA1303_INIT)
