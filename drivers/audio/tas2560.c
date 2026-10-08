/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tas2560

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "tas2560.h"
#include "tas2560_cfg.inc"

LOG_MODULE_REGISTER(tas2560, CONFIG_AUDIO_CODEC_LOG_LEVEL);

struct tas2560_config {
	struct i2c_dt_spec i2c;
	uint8_t asi_channel;
};

struct tas2560_data {
	struct k_mutex lock;
	bool started;
};

static int tas2560_load_playback(const struct device *dev)
{
	const struct tas2560_config *cfg = dev->config;

	for (size_t i = 0; i < ARRAY_SIZE(tas2560_playback_cfg); i++) {
		uint8_t reg = tas2560_playback_cfg[i][0];
		uint8_t val = tas2560_playback_cfg[i][1];
		int ret;

		if (reg == TAS2560_CFG_DELAY) {
			k_msleep(val);
			continue;
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int tas2560_select_page0(const struct device *dev)
{
	const struct tas2560_config *cfg = dev->config;
	int ret;

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PAGE, 0x00);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_BOOK, 0x00);
}

static int tas2560_configure(const struct device *dev, struct audio_codec_cfg *audio_cfg)
{
	const struct tas2560_config *cfg = dev->config;
	struct tas2560_data *data = dev->data;
	uint8_t spk;
	int ret;

	if (audio_cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("Only I2S is supported");
		return -EINVAL;
	}

	if (audio_cfg->dai_cfg.i2s.format != I2S_FMT_DATA_FORMAT_I2S) {
		LOG_ERR("Only I2S format is supported");
		return -EINVAL;
	}

	if (audio_cfg->dai_cfg.i2s.word_size != AUDIO_PCM_WIDTH_16_BITS) {
		LOG_ERR("Only 16-bit I2S is supported");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tas2560_select_page0(dev);
	if (ret < 0) {
		goto unlock;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_RESET, 0x01);
	if (ret < 0) {
		goto unlock;
	}

	k_msleep(10);

	ret = tas2560_select_page0(dev);
	if (ret < 0) {
		goto unlock;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, TAS2560_SPK_CTRL, &spk);
	if (ret < 0) {
		goto unlock;
	}

	if (spk != TAS2560_SPK_CTRL_RESET) {
		LOG_ERR("SPK_CTRL 0x%02x is not TAS2560", spk);
		ret = -ENODEV;
		goto unlock;
	}

	ret = tas2560_load_playback(dev);
	if (ret < 0) {
		goto unlock;
	}

	ret = tas2560_select_page0(dev);
	if (ret < 0) {
		goto unlock;
	}

	/*
	 * The DSP program expects PLL = 1024 * Fs. With BCLK as the PLL input
	 * that is J = 1024 / (word_size * channels). 16-bit stereo stays at
	 * J = 32 for every rate. BCLK below 1 MHz must set PLL_LOWF.
	 * Hold the amplifier off until BCLK is running.
	 */
	{
		uint8_t channels = audio_cfg->dai_cfg.i2s.channels;
		uint32_t freq = audio_cfg->dai_cfg.i2s.frame_clk_freq;
		uint32_t bits = audio_cfg->dai_cfg.i2s.word_size;
		uint32_t slot_bits = bits * channels;
		uint32_t bclk;
		uint32_t j;
		uint32_t d;
		uint8_t jreg;

		if (channels == 0 || slot_bits == 0 || freq == 0) {
			ret = -EINVAL;
			goto unlock;
		}

		bclk = freq * slot_bits;
		if (bclk < 512000 || bclk > 20000000) {
			LOG_ERR("BCLK %u Hz is outside 512 kHz..20 MHz", bclk);
			ret = -EINVAL;
			goto unlock;
		}

		j = 1024U / slot_bits;
		d = ((1024U % slot_bits) * 10000U) / slot_bits;
		if (j < 1 || j > 63) {
			LOG_ERR("PLL J %u is out of range", j);
			ret = -EINVAL;
			goto unlock;
		}

		jreg = (uint8_t)j;
		if (bclk < 1000000) {
			jreg |= BIT(7);
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PLL_JVAL, jreg);
		if (ret < 0) {
			goto unlock;
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PLL_DVAL_1, (d >> 8) & 0x3f);
		if (ret < 0) {
			goto unlock;
		}

		ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PLL_DVAL_2, d & 0xff);
		if (ret < 0) {
			goto unlock;
		}
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_ASI_FORMAT, 0);
	if (ret < 0) {
		goto unlock;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_ASI_CHANNEL, cfg->asi_channel);
	if (ret < 0) {
		goto unlock;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PWR_CTRL_1, TAS2560_PWR_SHUTDOWN);
	if (ret == 0) {
		data->started = false;
		LOG_INF("TAS2560 @ 0x%02x configured for %u Hz, %u-bit, slot %u", cfg->i2c.addr,
			audio_cfg->dai_cfg.i2s.frame_clk_freq, audio_cfg->dai_cfg.i2s.word_size,
			cfg->asi_channel);
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static void tas2560_start_output(const struct device *dev)
{
	const struct tas2560_config *cfg = dev->config;
	struct tas2560_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tas2560_select_page0(dev);
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PWR_CTRL_1, TAS2560_PWR_ACTIVE);
	}

	if (ret < 0) {
		LOG_ERR("Failed to start output: %d", ret);
	} else {
		data->started = true;
		LOG_INF("TAS2560 output started");
	}

	k_mutex_unlock(&data->lock);
}

static void tas2560_stop_output(const struct device *dev)
{
	const struct tas2560_config *cfg = dev->config;
	struct tas2560_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tas2560_select_page0(dev);
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PWR_CTRL_1, TAS2560_PWR_SHUTDOWN);
	}

	if (ret < 0) {
		LOG_ERR("Failed to stop output: %d", ret);
	} else {
		data->started = false;
	}

	k_mutex_unlock(&data->lock);
}

static int tas2560_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	const struct tas2560_config *cfg = dev->config;
	struct tas2560_data *data = dev->data;
	uint8_t gain;
	int ret;

	if (channel != AUDIO_CHANNEL_ALL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		gain = val.vol > 100 ? 15 : (val.vol * 15) / 100;
		ret = tas2560_select_page0(dev);
		if (ret == 0) {
			ret = i2c_reg_update_byte_dt(&cfg->i2c, TAS2560_SPK_CTRL, 0x0f, gain);
		}
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		ret = tas2560_select_page0(dev);
		if (ret == 0) {
			ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PWR_CTRL_1,
						    val.mute ? TAS2560_PWR_SHUTDOWN :
							       TAS2560_PWR_ACTIVE);
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static DEVICE_API(audio_codec, tas2560_driver_api) = {
	.configure = tas2560_configure,
	.start_output = tas2560_start_output,
	.stop_output = tas2560_stop_output,
	.set_property = tas2560_set_property,
};

static int tas2560_init(const struct device *dev)
{
	const struct tas2560_config *cfg = dev->config;
	struct tas2560_data *data = dev->data;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		return -ENODEV;
	}

	k_mutex_init(&data->lock);
	return 0;
}

#define TAS2560_INIT(inst)                                                                         \
	static struct tas2560_data tas2560_data_##inst;                                            \
	static const struct tas2560_config tas2560_cfg_##inst = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.asi_channel = DT_INST_PROP(inst, ti_asi_channel),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tas2560_init, NULL, &tas2560_data_##inst,                      \
			      &tas2560_cfg_##inst, POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, \
			      &tas2560_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TAS2560_INIT)
