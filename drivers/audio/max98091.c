/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Silicon Signals Pvt. Ltd.
 * Author: Rutvij Trivedi <rutvij.trivedi@siliconsignals.io>
 * Author: Tarang Raval <tarang.raval@siliconsignals.io>
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include "max98091.h"

LOG_MODULE_REGISTER(maxim_max98091);

#define DT_DRV_COMPAT maxim_max98091

struct max98091_config {
	struct i2c_dt_spec i2c;
	uint32_t mclk_freq;
};

static void max98091_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct max98091_config *const dev_cfg = dev->config;

	i2c_reg_write_byte_dt(&dev_cfg->i2c, reg, val);
}

static void max98091_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct max98091_config *const dev_cfg = dev->config;

	i2c_reg_read_byte_dt(&dev_cfg->i2c, reg, val);
}

static void max98091_update_reg(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	const struct max98091_config *const dev_cfg = dev->config;

	i2c_reg_update_byte_dt(&dev_cfg->i2c, reg, mask, val);
}

static void max98091_soft_reset(const struct device *dev)
{
	max98091_write_reg(dev, M98091_REG_SOFTWARE_RESET, 0x01);
	k_msleep(20);
}

/* Configuration Functions */
static int max98091_protocol_config(const struct device *dev, audio_dai_type_t dai_type)
{
	uint8_t fmt_reg = 0;

	switch (dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		fmt_reg |= M98091_I2S_S_MASK;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		fmt_reg |= M98091_LJ_S_MASK;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		fmt_reg |= M98091_RJ_S_MASK;
		break;
	default:
		LOG_ERR("Unsupported DAI type: %d", dai_type);
		return -EINVAL;
	}
	max98091_write_reg(dev, M98091_REG_DAI_INTERFACE, fmt_reg);
	LOG_DBG("Protocol configured: 0x%02x", fmt_reg);
	return 0;
}

static int max98091_audio_fmt_config(const struct device *dev, audio_dai_cfg_t *cfg)
{
	uint8_t sample_rate;
	uint8_t channels;
	uint8_t word_size;

	switch (cfg->i2s.frame_clk_freq) {
	case 8000:
		sample_rate = M98091_SR_8K_MASK;
		break;
	case 16000:
		sample_rate = M98091_SR_16K_MASK;
		break;
	case 32000:
		sample_rate = M98091_SR_32K_MASK;
		break;
	case 44100:
		sample_rate = M98091_SR_44K1_MASK;
		break;
	case 48000:
		sample_rate = M98091_SR_48K_MASK;
		break;
	case 96000:
		sample_rate = M98091_SR_96K_MASK;
		break;
	default:
		LOG_ERR("Unsupported sample rate: %d", cfg->i2s.frame_clk_freq);
		return -EINVAL;
	}

	max98091_write_reg(dev, M98091_REG_QUICK_SAMPLE_RATE, sample_rate);

	switch (cfg->i2s.channels) {
	case 1: /* Mono */
		channels = 1;
		break;
	case 2: /* Stereo */
		channels = 0;
		break;
	default:
		LOG_ERR("Unsupported channels: %d", cfg->i2s.channels);
		return -EINVAL;
	}
	max98091_update_reg(dev, M98091_REG_IO_CONFIGURATION, M98091_DMONO_MASK, channels);

	switch (cfg->i2s.word_size) {
	case 16:
		word_size = M98091_16B_WS;
		break;
	default:
		LOG_ERR("Word size %d bits not supported; falling back to 16 bits",
			cfg->i2s.word_size);
		word_size = M98091_16B_WS;
		break;
	}
	max98091_update_reg(dev, M98091_REG_INTERFACE_FORMAT, M98091_WS_MASK, word_size);

	return 0;
}

static void max98091_set_system_clock(const struct device *dev, uint32_t mclk_freq)
{
	uint8_t psclk;

	if (mclk_freq >= 10000000 && mclk_freq <= 20000000) {
		psclk = M98091_PSCLK_DIV1;
	} else if (mclk_freq > 20000000 && mclk_freq <= 40000000) {
		psclk = M98091_PSCLK_DIV2;
	} else if (mclk_freq > 40000000 && mclk_freq <= 60000000) {
		psclk = M98091_PSCLK_DIV4;
	} else {
		LOG_ERR("Invalid MCLK frequency: %u", mclk_freq);
		return;
	}
	max98091_write_reg(dev, M98091_REG_SYSTEM_CLOCK, psclk);
	LOG_DBG("System clock set: PSCLK=0x%02x", psclk);

	max98091_update_reg(dev, M98091_REG_MASTER_MODE, M98091_MAS_MASK, 0);
}

static int max98091_set_volume_or_mute(const struct device *dev, audio_channel_t channel, int value,
				      bool is_volume)
{
	uint8_t hp_mask = is_volume ? M98091_HPVOLL_MASK : M98091_HPLM_MASK;
	uint8_t spk_mask = is_volume ? M98091_SPVOLL_MASK : M98091_SPLM_MASK;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		max98091_update_reg(dev, M98091_REG_LEFT_SPK_VOLUME, spk_mask, value);
		return 0;

	case AUDIO_CHANNEL_FRONT_RIGHT:
		max98091_update_reg(dev, M98091_REG_RIGHT_SPK_VOLUME, spk_mask, value);
		return 0;

	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		max98091_update_reg(dev, M98091_REG_LEFT_HP_VOLUME, hp_mask, value);
		return 0;

	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		max98091_update_reg(dev, M98091_REG_RIGHT_HP_VOLUME, hp_mask, value);
		return 0;

	case AUDIO_CHANNEL_ALL:
		max98091_update_reg(dev, M98091_REG_LEFT_SPK_VOLUME, spk_mask, value);
		max98091_update_reg(dev, M98091_REG_RIGHT_SPK_VOLUME, spk_mask, value);
		max98091_update_reg(dev, M98091_REG_LEFT_HP_VOLUME, hp_mask, value);
		max98091_update_reg(dev, M98091_REG_RIGHT_HP_VOLUME, hp_mask, value);
		return 0;

	default:
		return -EINVAL;
	}
}

static int max98091_out_volume_config(const struct device *dev, audio_channel_t channel, int volume)
{
	return max98091_set_volume_or_mute(dev, channel, volume, true);
}

static int max98091_out_mute_config(const struct device *dev, audio_channel_t channel, bool mute)
{
	return max98091_set_volume_or_mute(dev, channel, mute, false);
}

/* Audio Path Configuration */
static void max98091_configure_output(const struct device *dev)
{
	max98091_update_reg(dev, M98091_REG_IO_CONFIGURATION, M98091_SDIEN_MASK, M98091_SDIEN_MASK);

	max98091_write_reg(dev, M98091_REG_LEFT_SPK_MIXER, M98091_MIXSPL_DACL_MASK);
	max98091_write_reg(dev, M98091_REG_RIGHT_SPK_MIXER, M98091_MIXSPR_DACR_MASK);

	/* select DAC only source */
	max98091_write_reg(dev, M98091_REG_HP_CONTROL, 0x00);

	/* M98091_HPREN_MASK, M98091_HPLEN_MASK, M98091_SPREN_MASK,
	 * M98091_SPLEN_MASK, M98091_DAREN_MASK, M98091_DALEN_MASK
	 */
	max98091_write_reg(dev, M98091_REG_OUTPUT_ENABLE, 0xf3);

	max98091_out_volume_config(dev, AUDIO_CHANNEL_ALL, M98091_DEFAULT_VOLUME);
	max98091_out_mute_config(dev, AUDIO_CHANNEL_ALL, false);
}

static void max98091_start_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void max98091_stop_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int max98091_set_property(const struct device *dev, audio_property_t property,
				 audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return max98091_out_volume_config(dev, channel, val.vol);
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return max98091_out_mute_config(dev, channel, val.mute);
	default:
		return -EINVAL;
	}
}

static int max98091_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct max98091_config *const dev_cfg = dev->config;

	if (cfg->dai_type >= AUDIO_DAI_TYPE_INVALID) {
		LOG_ERR("dai_type not supported");
		return -EINVAL;
	}

	max98091_soft_reset(dev);

	if (cfg->dai_route == AUDIO_ROUTE_BYPASS) {
		return 0;
	}

	/* Put the audio codec into shutdown mode */
	max98091_write_reg(dev, M98091_REG_DEVICE_SHUTDOWN, 0x00);

	max98091_write_reg(dev, M98091_REG_DAC_CONTROL, 0x00);

	max98091_write_reg(dev, M98091_REG_TDM_CONTROL, 0x00);

	/* Set DLY = 1 to conform to the I2S standard.
	 * DLY is only effective when TDM = 0
	 */
	max98091_write_reg(dev, M98091_REG_INTERFACE_FORMAT, M98091_DLY_MASK);

	/* Configure system clock */
	max98091_set_system_clock(dev, dev_cfg->mclk_freq);

	max98091_protocol_config(dev, cfg->dai_type);
	max98091_audio_fmt_config(dev, &cfg->dai_cfg);

	/* Configure audio paths based on route */
	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
		max98091_configure_output(dev);
		break;
	default:
		LOG_DBG("Unsupported audio route selected");
		break;
	}

	/* Bring the audio codec out of shutdown mode */
	max98091_write_reg(dev, M98091_REG_DEVICE_SHUTDOWN, M98091_SHDNN_MASK);

	return 0;
}

static const struct audio_codec_api max98091_api = {
	.configure = max98091_configure,
	.start_output = max98091_start_output,
	.stop_output = max98091_stop_output,
	.set_property = max98091_set_property,
};

static int max98091_init(const struct device *dev)
{
	const struct max98091_config *cfg_tan = dev->config;
	uint8_t device_id;

	if (!i2c_is_ready_dt(&cfg_tan->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	max98091_read_reg(dev, M98091_REG_REVISION_ID, &device_id);
	if (device_id >= M98091_REVA && (device_id <= M98091_REVA + 0x0f)) {
		LOG_INF("MAX98091 Device ID: 0x%02X", device_id);
		return 0;
	}

	LOG_ERR("Invalid MAX98091 Device ID: 0x%02X", device_id);
	return -EINVAL;
}

#define MAX98091_INIT(inst)								\
	static const struct max98091_config max98091_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
		.mclk_freq = DT_INST_PROP(inst, mclk_frequency)};			\
	DEVICE_DT_INST_DEFINE(inst, max98091_init, NULL, NULL, &max98091_config_##inst, \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &max98091_api);

DT_INST_FOREACH_STATUS_OKAY(MAX98091_INIT)
