/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max98357a

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(max98357a, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define MAX98357A_FRAME_CLK_FREQ_MIN 8000U
#define MAX98357A_FRAME_CLK_FREQ_MAX 96000U

struct max98357a_config {
	struct gpio_dt_spec sdmode;
};

static int max98357a_set_enabled(const struct device *dev, bool enabled)
{
	const struct max98357a_config *cfg = dev->config;

	if (cfg->sdmode.port == NULL) {
		return -ENOTSUP;
	}

	return gpio_pin_set_dt(&cfg->sdmode, enabled ? 1 : 0);
}

static int max98357a_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct i2s_config *i2s_cfg;
	uint8_t format;

	ARG_UNUSED(dev);

	if (cfg == NULL) {
		return -EINVAL;
	}

	if (cfg->dai_route != AUDIO_ROUTE_PLAYBACK) {
		LOG_ERR("Only playback is supported");
		return -ENOTSUP;
	}

	if ((cfg->dai_type != AUDIO_DAI_TYPE_I2S) &&
	    (cfg->dai_type != AUDIO_DAI_TYPE_LEFT_JUSTIFIED)) {
		LOG_ERR("Unsupported DAI type %d", cfg->dai_type);
		return -ENOTSUP;
	}

	i2s_cfg = &cfg->dai_cfg.i2s;
	format = i2s_cfg->format & I2S_FMT_DATA_FORMAT_MASK;

	if ((format != I2S_FMT_DATA_FORMAT_I2S) && (format != I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED)) {
		LOG_ERR("Unsupported data format 0x%02x", format);
		return -ENOTSUP;
	}

	if ((i2s_cfg->word_size != 16U) && (i2s_cfg->word_size != 24U) &&
	    (i2s_cfg->word_size != 32U)) {
		LOG_ERR("Unsupported word size %u", i2s_cfg->word_size);
		return -EINVAL;
	}

	if ((i2s_cfg->frame_clk_freq < MAX98357A_FRAME_CLK_FREQ_MIN) ||
	    (i2s_cfg->frame_clk_freq > MAX98357A_FRAME_CLK_FREQ_MAX)) {
		LOG_ERR("Unsupported sample rate %u", i2s_cfg->frame_clk_freq);
		return -EINVAL;
	}

	return 0;
}

static void max98357a_start_output(const struct device *dev)
{
	(void)max98357a_set_enabled(dev, true);
}

static void max98357a_stop_output(const struct device *dev)
{
	(void)max98357a_set_enabled(dev, false);
}

static int max98357a_set_property(const struct device *dev, audio_property_t property,
				  audio_channel_t channel, audio_property_value_t val)
{
	ARG_UNUSED(channel);

	if (property != AUDIO_PROPERTY_OUTPUT_MUTE) {
		return -ENOTSUP;
	}

	return max98357a_set_enabled(dev, !val.mute);
}

static int max98357a_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int max98357a_init(const struct device *dev)
{
	const struct max98357a_config *cfg = dev->config;
	int ret;

	if (cfg->sdmode.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->sdmode)) {
		LOG_ERR("SD_MODE GPIO port %s is not ready", cfg->sdmode.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->sdmode, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure SD_MODE GPIO: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(audio_codec, max98357a_api) = {
	.configure = max98357a_configure,
	.start_output = max98357a_start_output,
	.stop_output = max98357a_stop_output,
	.set_property = max98357a_set_property,
	.apply_properties = max98357a_apply_properties,
};

#define MAX98357A_DEFINE(inst)                                                                     \
	static const struct max98357a_config max98357a_config_##inst = {                           \
		.sdmode = GPIO_DT_SPEC_INST_GET_OR(inst, sdmode_gpios, {0}),                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max98357a_init, NULL, NULL, &max98357a_config_##inst,          \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &max98357a_api);

DT_INST_FOREACH_STATUS_OKAY(MAX98357A_DEFINE)
