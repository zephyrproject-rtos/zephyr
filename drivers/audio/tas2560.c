/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tas2560

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "tas2560.h"

LOG_MODULE_REGISTER(tas2560, CONFIG_AUDIO_CODEC_LOG_LEVEL);

/*
 * Book 0 DSP coefficients. Section 9.6.1 writes each coefficient as one
 * 4-byte I2C burst. The image does not change book, and it does not unmute.
 */
struct tas2560_dsp_burst {
	uint8_t page;
	uint8_t reg;
	uint8_t data[4];
};

static const struct tas2560_dsp_burst tas2560_dsp_cfg[] = {
	{0x32, 0x28, {0x7f, 0xfb, 0xb5, 0x00}},
	{0x32, 0x2c, {0x80, 0x04, 0x4c, 0x00}},
	{0x32, 0x30, {0x7f, 0xf7, 0x6a, 0x00}},
	{0x32, 0x1c, {0x7f, 0xff, 0xff, 0xff}},
	{0x32, 0x20, {0x00, 0x00, 0x00, 0x00}},
	{0x32, 0x24, {0x00, 0x00, 0x00, 0x00}},
	{0x33, 0x18, {0x06, 0x66, 0x66, 0x66}},
	{0x34, 0x34, {0x3a, 0x46, 0x74, 0x00}},
	{0x34, 0x38, {0x22, 0xf3, 0x07, 0x00}},
	{0x34, 0x3c, {0x80, 0x77, 0x61, 0x00}},
	{0x34, 0x40, {0x22, 0xa7, 0xcc, 0x00}},
	{0x34, 0x44, {0x3a, 0x0c, 0x93, 0x00}},
	{0x33, 0x10, {0x75, 0xc2, 0x8e, 0x00}},
	{0x33, 0x14, {0x6e, 0x14, 0x79, 0x00}},
	{0x33, 0x20, {0x1e, 0x00, 0x00, 0x00}},
	{0x33, 0x24, {0x21, 0x3d, 0x71, 0x00}},
	{0x33, 0x28, {0x24, 0x7a, 0xe1, 0x00}},
	{0x33, 0x2c, {0x27, 0xb8, 0x52, 0x00}},
	{0x33, 0x30, {0x2a, 0xf5, 0xc3, 0x00}},
	{0x33, 0x34, {0x2e, 0x33, 0x33, 0x00}},
	{0x33, 0x38, {0x31, 0x70, 0xa4, 0x00}},
	{0x33, 0x3c, {0x34, 0xae, 0x14, 0x00}},
	{0x32, 0x3c, {0x3d, 0x99, 0x9a, 0x00}},
	{0x32, 0x40, {0x30, 0x00, 0x00, 0x00}},
	{0x32, 0x44, {0x50, 0x00, 0x00, 0x00}},
	{0x32, 0x4c, {0x02, 0x00, 0x00, 0x00}},
	{0x32, 0x54, {0x00, 0x88, 0x40, 0x00}},
	{0x32, 0x58, {0x00, 0x06, 0xd3, 0x00}},
	{0x32, 0x60, {0x28, 0x00, 0x00, 0x00}},
	{0x33, 0x40, {0x00, 0x00, 0x00, 0x00}},
	{0x33, 0x64, {0x39, 0x80, 0x00, 0x00}},
};

struct tas2560_config {
	struct i2c_dt_spec i2c;
	uint8_t asi_channel;
};

struct tas2560_data {
	struct k_mutex lock;
	bool started;
};

struct tas2560_clock {
	uint8_t jreg;
	uint8_t d_msb;
	uint8_t d_lsb;
	uint8_t pcm_rate;
};

static int tas2560_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct tas2560_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
}

static int tas2560_select_page(const struct device *dev, uint8_t page)
{
	return tas2560_write(dev, TAS2560_PAGE, page);
}

static int tas2560_clock_params(const struct audio_codec_cfg *audio_cfg,
				struct tas2560_clock *pll)
{
	uint8_t channels = audio_cfg->dai_cfg.i2s.channels;
	uint32_t freq = audio_cfg->dai_cfg.i2s.frame_clk_freq;
	uint32_t bits = audio_cfg->dai_cfg.i2s.word_size;
	uint32_t slot_bits = bits * channels;
	uint32_t bclk;
	uint32_t j;
	uint32_t d;

	if (channels == 0 || slot_bits == 0 || freq == 0) {
		return -EINVAL;
	}

	switch (freq) {
	case 8000:
		pll->pcm_rate = TAS2560_PCM_RATE_8KHZ;
		break;
	case 16000:
		pll->pcm_rate = TAS2560_PCM_RATE_16KHZ;
		break;
	case 48000:
		pll->pcm_rate = TAS2560_PCM_RATE_48KHZ;
		break;
	case 96000:
		pll->pcm_rate = TAS2560_PCM_RATE_96KHZ;
		break;
	default:
		LOG_ERR("Unsupported sample rate: %u", freq);
		return -EINVAL;
	}

	bclk = freq * slot_bits;
	if (bclk < 512000 || bclk > 20000000) {
		LOG_ERR("BCLK %u Hz is outside 512 kHz..20 MHz", bclk);
		return -EINVAL;
	}

	/*
	 * The DSP program expects PLL = 1024 * Fs. With BCLK as the PLL input
	 * that is J = 1024 / (word_size * channels).
	 */
	j = 1024U / slot_bits;
	d = ((1024U % slot_bits) * 10000U) / slot_bits;
	if (j < 1 || j > 63) {
		LOG_ERR("PLL J %u is out of range", j);
		return -EINVAL;
	}

	pll->jreg = (uint8_t)j;
	if (bclk < 1000000) {
		pll->jreg |= TAS2560_PLL_LOWF;
	}

	pll->d_msb = (d >> 8) & 0x3f;
	pll->d_lsb = d & 0xff;

	return 0;
}

/* Book 0 controls that differ from reset. The speaker stays muted. */
static int tas2560_program_controls(const struct device *dev)
{
	int ret;

	ret = tas2560_select_page(dev, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = tas2560_write(dev, TAS2560_BOOK, 0x00);
	if (ret < 0) {
		return ret;
	}

	ret = tas2560_write(dev, TAS2560_PLL_CLKIN, TAS2560_PLL_CLKIN_BCLK);
	if (ret < 0) {
		return ret;
	}

	ret = tas2560_write(dev, TAS2560_CLOCK_ERR_CFG_2, TAS2560_CLOCK_ERR_PLAY);
	if (ret < 0) {
		return ret;
	}

	ret = tas2560_write(dev, TAS2560_SPK_CTRL, TAS2560_SPK_CTRL_PLAY);
	if (ret < 0) {
		return ret;
	}

	/* Section 9.6.1 init. These addresses are not in the summary table. */
	ret = tas2560_write(dev, 0x49, 0x0c);
	if (ret < 0) {
		return ret;
	}

	ret = tas2560_write(dev, 0x3c, 0x33);
	if (ret < 0) {
		return ret;
	}

	ret = tas2560_write(dev, TAS2560_MODE, TAS2560_DSP_MODE_PCM_IV);
	if (ret < 0) {
		return ret;
	}

	/* 9.6.1 powers up muted before the DSP patch. */
	ret = tas2560_write(dev, TAS2560_PWR_CTRL_1, TAS2560_PWR_MUTE);
	if (ret < 0) {
		return ret;
	}

	k_msleep(TAS2560_RESET_WAIT_MS);

	return 0;
}

static int tas2560_load_dsp(const struct device *dev)
{
	const struct tas2560_config *cfg = dev->config;
	uint8_t page = 0xff;
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(tas2560_dsp_cfg); i++) {
		if (tas2560_dsp_cfg[i].page != page) {
			ret = tas2560_select_page(dev, tas2560_dsp_cfg[i].page);
			if (ret < 0) {
				return ret;
			}
			page = tas2560_dsp_cfg[i].page;
		}

		ret = i2c_burst_write_dt(&cfg->i2c, tas2560_dsp_cfg[i].reg, tas2560_dsp_cfg[i].data,
					 sizeof(tas2560_dsp_cfg[i].data));
		if (ret < 0) {
			return ret;
		}
	}

	/* The patch leaves a DSP page. Book 0 was never changed. */
	return tas2560_select_page(dev, 0x00);
}

static int tas2560_set_pll(const struct device *dev, const struct tas2560_clock *pll)
{
	const struct tas2560_config *cfg = dev->config;
	int ret;

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PLL_JVAL, pll->jreg);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PLL_DVAL_1, pll->d_msb);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PLL_DVAL_2, pll->d_lsb);
}

static int tas2560_configure(const struct device *dev, struct audio_codec_cfg *audio_cfg)
{
	const struct tas2560_config *cfg = dev->config;
	struct tas2560_data *data = dev->data;
	struct tas2560_clock pll;
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

	ret = tas2560_clock_params(audio_cfg, &pll);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = tas2560_select_page(dev, 0x00);
	if (ret < 0) {
		goto unlock;
	}

	ret = tas2560_write(dev, TAS2560_BOOK, 0x00);
	if (ret < 0) {
		goto unlock;
	}

	ret = tas2560_write(dev, TAS2560_RESET, 0x01);
	if (ret < 0) {
		goto unlock;
	}

	/* The part is back in reset, so a later failure must not skip start. */
	data->started = false;

	/*
	 * The reset bit is self-clearing. The datasheet requires 50 us for OTP.
	 * The playback script validated on hardware waits 16 ms.
	 */
	k_msleep(TAS2560_RESET_WAIT_MS);

	ret = i2c_reg_read_byte_dt(&cfg->i2c, TAS2560_SPK_CTRL, &spk);
	if (ret < 0) {
		goto unlock;
	}

	if (spk != TAS2560_SPK_CTRL_RESET) {
		LOG_ERR("SPK_CTRL 0x%02x is not TAS2560", spk);
		ret = -ENODEV;
		goto unlock;
	}

	ret = tas2560_program_controls(dev);
	if (ret < 0) {
		goto unlock;
	}

	ret = tas2560_load_dsp(dev);
	if (ret < 0) {
		goto unlock;
	}

	ret = tas2560_set_pll(dev, &pll);
	if (ret < 0) {
		goto unlock;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, TAS2560_PCM_RATE, TAS2560_PCM_RATE_MASK,
				     pll.pcm_rate);
	if (ret < 0) {
		goto unlock;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_ASI_FORMAT, TAS2560_ASI_I2S_16);
	if (ret < 0) {
		goto unlock;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_ASI_CHANNEL, cfg->asi_channel);
	if (ret < 0) {
		goto unlock;
	}

	/* Hold the amplifier off until BCLK is running. */
	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PWR_CTRL_1, TAS2560_PWR_SHUTDOWN);
	if (ret == 0) {
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

	if (data->started) {
		k_mutex_unlock(&data->lock);
		return;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PWR_CTRL_1, TAS2560_PWR_ACTIVE);
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

	if (!data->started) {
		k_mutex_unlock(&data->lock);
		return;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, TAS2560_PWR_CTRL_1, TAS2560_PWR_SHUTDOWN);
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
		gain = (uint8_t)((CLAMP(val.vol, 0, 100) * 15) / 100);
		ret = i2c_reg_update_byte_dt(&cfg->i2c, TAS2560_SPK_CTRL, 0x0f, gain);
		break;
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		ret = i2c_reg_update_byte_dt(&cfg->i2c, TAS2560_PWR_CTRL_1, TAS2560_MUTE_AUDIO,
					     val.mute ? TAS2560_MUTE_AUDIO : 0);
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
