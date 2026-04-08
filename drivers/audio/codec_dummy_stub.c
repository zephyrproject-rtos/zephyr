/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_codec_dummy_stub

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/audio/audio_caps.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

struct dummy_codec_data {
	struct audio_codec_cfg cfg;
	bool configured;
};

static int dummy_codec_validate_cfg(const struct audio_codec_cfg *cfg)
{
	const struct i2s_config *i2s_cfg;

	if (cfg == NULL) {
		return -EINVAL;
	}

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		return -ENOTSUP;
	}

	if ((cfg->dai_route != AUDIO_ROUTE_PLAYBACK) &&
	    (cfg->dai_route != AUDIO_ROUTE_PLAYBACK_CAPTURE) &&
	    (cfg->dai_route != AUDIO_ROUTE_BYPASS)) {
		return -ENOTSUP;
	}

	i2s_cfg = &cfg->dai_cfg.i2s;

	if ((i2s_cfg->word_size != AUDIO_PCM_WIDTH_16_BITS) || (i2s_cfg->channels != 2U) ||
	    (i2s_cfg->frame_clk_freq != AUDIO_PCM_RATE_48K)) {
		return -ENOTSUP;
	}

	if (i2s_cfg->format != I2S_FMT_DATA_FORMAT_I2S) {
		return -ENOTSUP;
	}

	return 0;
}

static int dummy_codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct dummy_codec_data *data = dev->data;
	int ret;

	ret = dummy_codec_validate_cfg(cfg);
	if (ret < 0) {
		return ret;
	}

	data->cfg = *cfg;
	data->configured = true;

	return 0;
}

static int dummy_codec_get_caps(const struct device *dev, struct audio_caps *caps)
{
	ARG_UNUSED(dev);

	if (caps == NULL) {
		return -EINVAL;
	}

	memset(caps, 0, sizeof(*caps));
	caps->min_total_channels = 2U;
	caps->max_total_channels = 2U;
	caps->supported_sample_rates = AUDIO_SAMPLE_RATE_48000;
	caps->supported_bit_widths = AUDIO_BIT_WIDTH_16;
	caps->min_num_buffers = 2U;
	caps->min_frame_interval = 1U;
	caps->max_frame_interval = UINT32_MAX;
	caps->interleaved = true;

	return 0;
}

static int dummy_codec_init(const struct device *dev)
{
	struct dummy_codec_data *data = dev->data;

	memset(data, 0, sizeof(*data));
	return 0;
}

static const struct audio_codec_api dummy_codec_api = {
	.configure = dummy_codec_configure,
	.get_caps = dummy_codec_get_caps,
};

/* clang-format off */
#define DUMMY_CODEC_DEFINE(inst) \
	static struct dummy_codec_data dummy_codec_data_##inst; \
	DEVICE_DT_INST_DEFINE(inst, dummy_codec_init, NULL, \
			      &dummy_codec_data_##inst, NULL, \
			      POST_KERNEL, \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, \
			      &dummy_codec_api)
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(DUMMY_CODEC_DEFINE)
