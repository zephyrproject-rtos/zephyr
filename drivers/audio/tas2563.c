/*
 * Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tas2563

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/logging/log.h>

#include "tas2563.h"

LOG_MODULE_REGISTER(tas2563, CONFIG_AUDIO_CODEC_LOG_LEVEL);
struct tas2563_config {
	struct i2c_dt_spec i2c;
};

struct tas2563_data {
	struct k_mutex lock;
	bool is_started;
};

static int tas2563_configure_tdm(const struct device *dev,
				 const struct audio_codec_cfg *cfg)
{
	const struct tas2563_config *config = dev->config;
	uint8_t tdm_cfg0 = 0, tdm_cfg1 = 0, tdm_cfg2 = 0;
	int ret;

	switch (cfg->dai_cfg.i2s.frame_clk_freq) {
	case 8000:
		tdm_cfg0 |= FIELD_PREP(TAS2563_SAMP_RATE_MASK, TAS2563_SR_8KHZ);
		break;
	case 16000:
		tdm_cfg0 |= FIELD_PREP(TAS2563_SAMP_RATE_MASK, TAS2563_SR_16KHZ);
		break;
	case 32000:
		tdm_cfg0 |= FIELD_PREP(TAS2563_SAMP_RATE_MASK, TAS2563_SR_32KHZ);
		break;
	case 48000:
		tdm_cfg0 |= FIELD_PREP(TAS2563_SAMP_RATE_MASK, TAS2563_SR_48KHZ);
		break;
	case 96000:
		tdm_cfg0 |= FIELD_PREP(TAS2563_SAMP_RATE_MASK, TAS2563_SR_96KHZ);
		break;
	case 192000:
		tdm_cfg0 |= FIELD_PREP(TAS2563_SAMP_RATE_MASK, TAS2563_SR_192KHZ);
		break;
	default:
		LOG_ERR("Unsupported sample rate: %d", cfg->dai_cfg.i2s.frame_clk_freq);
		return -EINVAL;
	}

	switch (cfg->dai_cfg.i2s.word_size) {
	case AUDIO_PCM_WIDTH_16_BITS:
		tdm_cfg2 |= FIELD_PREP(TAS2563_RX_WLEN_MASK, TAS2563_RX_WLEN_16BITS);
		tdm_cfg2 |= TAS2563_RX_SLEN_16BITS;
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		tdm_cfg2 |= FIELD_PREP(TAS2563_RX_WLEN_MASK, TAS2563_RX_WLEN_24BITS);
		tdm_cfg2 |= TAS2563_RX_SLEN_32BITS;
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		tdm_cfg2 |= FIELD_PREP(TAS2563_RX_WLEN_MASK, TAS2563_RX_WLEN_32BITS);
		tdm_cfg2 |= TAS2563_RX_SLEN_32BITS;
		break;
	default:
		LOG_ERR("Unsupported word size: %d", cfg->dai_cfg.i2s.word_size);
		return -EINVAL;
	}

	switch (cfg->dai_cfg.i2s.format) {
	case I2S_FMT_DATA_FORMAT_I2S:
		tdm_cfg0 |= TAS2563_FRAME_START;
		tdm_cfg1 |= FIELD_PREP(TAS2563_RX_OFFSET_MASK, 1);
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		tdm_cfg0 &= ~TAS2563_FRAME_START;
		tdm_cfg1 &= ~TAS2563_RX_OFFSET_MASK;
		break;
	default:
		LOG_ERR("Unsupported format: %d", cfg->dai_cfg.i2s.format);
		return -EINVAL;
	}

	tdm_cfg2 |= FIELD_PREP(TAS2563_RX_SCFG_MASK, TAS2563_RX_SCFG_MONO_LEFT);

	ret = i2c_reg_write_byte_dt(&config->i2c, TAS2563_TDM_CFG0, tdm_cfg0);
	if (ret < 0) {
		LOG_ERR("Failed to write TDM_CFG0: %d", ret);
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, TAS2563_TDM_CFG1, tdm_cfg1);
	if (ret < 0) {
		LOG_ERR("Failed to write TDM_CFG1: %d", ret);
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, TAS2563_TDM_CFG2,
				     TAS2563_CFG2_CONFIG_MASK, tdm_cfg2);
	if (ret < 0) {
		LOG_ERR("Failed to write TDM_CFG2: %d", ret);
		return ret;
	}

	LOG_INF("TDM configured: word_size=%d bits", cfg->dai_cfg.i2s.word_size);

	return 0;
}

static int tas2563_configure(const struct device *dev,
			     struct audio_codec_cfg *cfg)
{
	const struct tas2563_config *config = dev->config;
	struct tas2563_data *data = dev->data;
	int ret;
	uint8_t rev_id;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("Only AUDIO_DAI_TYPE_I2S supported");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = i2c_reg_write_byte_dt(&config->i2c, TAS2563_SW_RESET, TAS2563_SW_RESET_BIT);
	if (ret < 0) {
		LOG_ERR("Software reset failed: %d", ret);
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, TAS2563_REV_ID, &rev_id);
	if (ret < 0) {
		LOG_ERR("Failed to read chip revision: %d", ret);
		return ret;
	}
	LOG_INF("TAS2563 chip revision: 0x%02X", rev_id);

	ret = i2c_reg_update_byte_dt(&config->i2c, TAS2563_PWR_CTL,
				     TAS2563_PWR_MODE_MASK,
				     TAS2563_PWR_MODE_SHUTDOWN);
	if (ret < 0) {
		LOG_ERR("Failed to set shutdown mode: %d", ret);
		return ret;
	}

	ret = tas2563_configure_tdm(dev, cfg);
	if (ret < 0) {
		goto unlock;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, TAS2563_PB_CFG1,
				     TAS2563_AMP_LEVEL_MASK,
				     FIELD_PREP(TAS2563_AMP_LEVEL_MASK,
						TAS2563_AMP_LEVEL_16_0DBV));
	if (ret < 0) {
		LOG_ERR("Failed to set amp level: %d", ret);
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static void tas2563_start_output(const struct device *dev)
{
	const struct tas2563_config *config = dev->config;
	struct tas2563_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->is_started) {
		LOG_WRN("Output already started");
		goto unlock;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, TAS2563_PWR_CTL,
				     TAS2563_PWR_MODE_MASK,
				     TAS2563_PWR_MODE_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to start output: %d", ret);
		goto unlock;
	}

	data->is_started = true;
	LOG_INF("Audio output started");

unlock:
	k_mutex_unlock(&data->lock);
}

static void tas2563_stop_output(const struct device *dev)
{
	const struct tas2563_config *config = dev->config;
	struct tas2563_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->is_started) {
		LOG_WRN("Output already stopped");
		goto unlock;
	}

	ret = i2c_reg_update_byte_dt(&config->i2c, TAS2563_PWR_CTL,
				     TAS2563_PWR_MODE_MASK,
				     TAS2563_PWR_MODE_SHUTDOWN);
	if (ret < 0) {
		LOG_ERR("Failed to stop output: %d", ret);
		goto unlock;
	}

	data->is_started = false;
	LOG_INF("Audio output stopped");

unlock:
	k_mutex_unlock(&data->lock);
}

static int tas2563_set_property(const struct device *dev,
				audio_property_t property,
				audio_channel_t channel,
				audio_property_value_t val)
{
	const struct tas2563_config *config = dev->config;
	struct tas2563_data *data = dev->data;
	uint8_t amp_level;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		if (channel != AUDIO_CHANNEL_ALL) {
			ret = -EINVAL;
			goto unlock;
		}
		if (val.vol <= 0) {
			amp_level = TAS2563_AMP_LEVEL_8_5DBV;
		} else if (val.vol >= 100) {
			amp_level = TAS2563_AMP_LEVEL_22_0DBV;
		} else {
			amp_level = TAS2563_AMP_LEVEL_8_5DBV + (val.vol *
				    (TAS2563_AMP_LEVEL_22_0DBV - TAS2563_AMP_LEVEL_8_5DBV)) / 100;
		}

		ret = i2c_reg_update_byte_dt(&config->i2c, TAS2563_PB_CFG1,
					     TAS2563_AMP_LEVEL_MASK,
					     FIELD_PREP(TAS2563_AMP_LEVEL_MASK, amp_level));
		if (ret < 0) {
			LOG_ERR("Failed to set volume: %d", ret);
		}
		break;

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (channel != AUDIO_CHANNEL_ALL) {
			ret = -EINVAL;
			goto unlock;
		}

		if (val.mute) {
			ret = i2c_reg_update_byte_dt(&config->i2c, TAS2563_PWR_CTL,
						     TAS2563_PWR_MODE_MASK,
						     TAS2563_PWR_MODE_MUTE);
		} else {
			if (!data->is_started) {
				LOG_WRN("Cannot unmute: output not started");
				ret = -EINVAL;
				goto unlock;
			}
			ret = i2c_reg_update_byte_dt(&config->i2c, TAS2563_PWR_CTL,
						     TAS2563_PWR_MODE_MASK,
						     TAS2563_PWR_MODE_ACTIVE);
		}
		if (ret < 0) {
			LOG_ERR("Failed to set mute: %d", ret);
		}
		break;

	default:
		LOG_WRN("Unsupported property: %d", property);
		ret = -ENOTSUP;
		break;
	}
unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static const struct audio_codec_api tas2563_driver_api = {
	.configure = tas2563_configure,
	.start_output = tas2563_start_output,
	.stop_output = tas2563_stop_output,
	.set_property = tas2563_set_property,
};

static int tas2563_initialize(const struct device *dev)
{
	const struct tas2563_config *config = dev->config;
	struct tas2563_data *data = dev->data;

	LOG_INF("Initializing TAS2563 codec");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	data->is_started = false;

	return 0;
}

#define TAS2563_DEVICE_INIT(inst)                                         \
	static struct tas2563_data tas2563_data_##inst;                   \
									  \
	static const struct tas2563_config tas2563_config_##inst = {      \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                        \
	};                                                                \
									  \
	DEVICE_DT_INST_DEFINE(inst,                                       \
			tas2563_initialize,                               \
			NULL,                                             \
			&tas2563_data_##inst,                             \
			&tas2563_config_##inst,                           \
			POST_KERNEL,                                      \
			CONFIG_AUDIO_CODEC_INIT_PRIORITY,                 \
			&tas2563_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TAS2563_DEVICE_INIT)
