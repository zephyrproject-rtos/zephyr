/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_taa3020

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "taa3020.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(taa3020, CONFIG_AUDIO_CODEC_LOG_LEVEL);

struct codec_channel_config {
	uint8_t channel;

	enum taa3020_channel_in_type in_type;
	enum taa3020_channel_in_src in_src;
	enum taa3020_channel_coupling coupling;
	enum taa3020_channel_impedance impedance;
	bool agc_en;
};

struct codec_driver_config {
	struct i2c_dt_spec bus;
	const struct codec_channel_config *channels;
	size_t num_channels;

	bool areg_internal;
};

struct codec_channel_data {
	bool configured;
	audio_channel_t audio_channel;
	uint8_t volume;
	bool muted;
};

struct codec_driver_data {
	struct codec_channel_data channels[4];
};

static void codec_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct codec_driver_config *const config = dev->config;

	i2c_reg_write_byte_dt(&config->bus, reg, val);
	LOG_DBG("%s WR REG:0x%02x VAL:0x%02x", dev->name, reg, val);
}

static void codec_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct codec_driver_config *const config = dev->config;

	i2c_reg_read_byte_dt(&config->bus, reg, val);
	LOG_DBG("%s RD REG:0x%02x VAL:0x%02x", dev->name, reg, *val);
}

static void codec_set_device_page(const struct device *dev, uint8_t page)
{
	codec_write_reg(dev, TAA3020_PAGE_CFG, page);
}

static void codec_soft_reset(const struct device *dev)
{
	uint8_t val = TAA3020_SW_RESET_SW_RESET;

	codec_set_device_page(dev, 0);
	codec_write_reg(dev, TAA3020_SW_RESET, val);
}

static void codec_set_power(const struct device *dev, bool power_en)
{
	uint8_t val = 0;

	codec_read_reg(dev, TAA3020_PWR_CFG, &val);
	if (power_en) {
		val |= TAA3020_PWR_CFG_ADC_PDZ;
		val |= TAA3020_PWR_CFG_PLL_PDZ;
		val |= TAA3020_PWR_CFG_DYN_CH_PUPD_EN;
	} else {
		val &= ~(TAA3020_PWR_CFG_ADC_PDZ | TAA3020_PWR_CFG_PLL_PDZ |
			 TAA3020_PWR_CFG_DYN_CH_PUPD_EN);
	}
	codec_write_reg(dev, TAA3020_PWR_CFG, val);
}

static void codec_channel_enable(const struct device *dev, uint8_t channel, bool enable)
{
	uint8_t val = 0;
	struct codec_driver_data *data = dev->data;

	codec_read_reg(dev, TAA3020_ASI_OUT_CH_EN, &val);
	if (enable) {
		val |= TAA3020_ASI_OUT_CH_EN_CHANNEL(channel) & TAA3020_ASI_OUT_CH_EN_MASK;
	} else {
		val &= ~(TAA3020_ASI_OUT_CH_EN_CHANNEL(channel) & TAA3020_ASI_OUT_CH_EN_MASK);
	}
	codec_write_reg(dev, TAA3020_ASI_OUT_CH_EN, val);
	data->channels[channel].configured = enable;
}

static void codec_channel_set_volume(const struct device *dev, uint8_t channel, uint8_t volume)
{
	uint8_t ch_cfg2;

	switch (channel) {
	case 0:
		ch_cfg2 = TAA3020_CH1_CFG2;
		break;
	case 1:
		ch_cfg2 = TAA3020_CH2_CFG2;
		break;
	case 2:
		ch_cfg2 = TAA3020_CH3_CFG2;
		break;
	case 3:
		ch_cfg2 = TAA3020_CH4_CFG2;
		break;
	default:
		return;
	}

	codec_write_reg(dev, ch_cfg2, volume);
}

static int codec_configure_channel(const struct device *dev,
				   const struct codec_channel_config *chan_config)
{
	uint8_t ch_cfg0_addr;
	uint8_t val = 0;

	if (chan_config->channel > 2 && chan_config->in_src != TAA3020_CHANNEL_IN_SRC_PDM) {
		LOG_ERR("Channel %" PRIu8 " can only be configured as PDM in",
			chan_config->channel);
		return -EINVAL;
	}

	switch (chan_config->channel) {
	case 0:
		ch_cfg0_addr = TAA3020_CH1_CFG0;
		break;
	case 1:
		ch_cfg0_addr = TAA3020_CH2_CFG0;
		break;
	default:
		return 0;
	}

	switch (chan_config->in_type) {
	case TAA3020_CHANNEL_IN_TYPE_MICROPHONE:
		val |= TAA3020_CHx_CFG0_INTYP_MICROPHONE;
		break;
	case TAA3020_CHANNEL_IN_TYPE_LINE:
		val |= TAA3020_CHx_CFG0_INTYP_LINE;
		break;
	}

	switch (chan_config->in_src) {
	case TAA3020_CHANNEL_IN_SRC_ANALOG_DIFFERENTIAL:
		val |= TAA3020_CHx_CFG0_INSRC_ANALOG_DIFFERENTIAL;
		break;
	case TAA3020_CHANNEL_IN_SRC_ANALOG_SIGNLE_ENDED:
		val |= TAA3020_CHx_CFG0_INSRC_ANALOG_SINGLE_ENDED;
		break;
	case TAA3020_CHANNEL_IN_SRC_PDM:
		val |= TAA3020_CHx_CFG0_INSRC_PDM;
		break;
	}

	switch (chan_config->coupling) {
	case TAA3020_CHANNEL_COUPLING_AC:
		val &= ~TAA3020_CHx_CFG0_DC;
		break;
	case TAA3020_CHANNEL_COUPLING_DC:
		val |= TAA3020_CHx_CFG0_DC;
		break;
	}

	switch (chan_config->impedance) {
	case TAA3020_CHANNEL_IMPEDANCE_2_5K:
		val |= TAA3020_CHx_CFG0_IMPEDANCE_2_5K;
		break;
	case TAA3020_CHANNEL_IMPEDANCE_10K:
		val |= TAA3020_CHx_CFG0_IMPEDANCE_10K;
		break;
	case TAA3020_CHANNEL_IMPEDANCE_20K:
		val |= TAA3020_CHx_CFG0_IMPEDANCE_20K;
		break;
	}

	if (chan_config->agc_en) {
		val |= TAA3020_CHx_CFG0_AGCEN;
	}

	codec_write_reg(dev, ch_cfg0_addr, val);

	return 0;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	uint8_t val = 0;

	switch (cfg->dai_cfg.i2s.format) {
	case I2S_FMT_DATA_FORMAT_I2S:
		val |= TAA3020_ASI_CFG0_ASI_FORMAT_I2S;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		val |= TAA3020_ASI_CFG0_ASI_FORMAT_LEFT_JUSTIFIED;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->dai_cfg.i2s.word_size) {
	case 16:
		val |= TAA3020_ASI_CFG0_ASI_WLEN_16BIT;
		break;
	case 20:
		val |= TAA3020_ASI_CFG0_ASI_WLEN_20BIT;
		break;
	case 24:
		val |= TAA3020_ASI_CFG0_ASI_WLEN_24BIT;
		break;
	case 32:
		val |= TAA3020_ASI_CFG0_ASI_WLEN_32BIT;
		break;
	default:
		return -ENOTSUP;
	}

	codec_write_reg(dev, TAA3020_ASI_CFG0, val);

	codec_set_power(dev, true);

	return 0;
}

static int codec_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	uint8_t asi_ch;
	struct codec_driver_data *data = dev->data;
	uint8_t val = 0;

	switch (input) {
	case 0:
		asi_ch = TAA3020_ASI_CH1;
		break;
	case 1:
		asi_ch = TAA3020_ASI_CH2;
		break;
	case 2:
		asi_ch = TAA3020_ASI_CH3;
		break;
	case 3:
		asi_ch = TAA3020_ASI_CH4;
		break;
	default:
		return -EINVAL;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		val = TAA3020_ASI_CHx_SLOT(0);
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		val = TAA3020_ASI_CHx_RIGHT | TAA3020_ASI_CHx_SLOT(0);
		break;
	case AUDIO_CHANNEL_REAR_LEFT:
		val = TAA3020_ASI_CHx_SLOT(1);
		break;
	case AUDIO_CHANNEL_REAR_RIGHT:
		val = TAA3020_ASI_CHx_RIGHT | TAA3020_ASI_CHx_SLOT(1);
		break;
	default:
		return -ENOTSUP;
	}

	for (size_t i = 0; i < ARRAY_SIZE(data->channels); i++) {
		if (data->channels[i].configured && data->channels[i].audio_channel == channel) {
			codec_channel_enable(dev, i, false);
		}
	}

	codec_write_reg(dev, asi_ch, val);
	data->channels[input].audio_channel = channel;
	codec_channel_enable(dev, input, true);

	return 0;
}

static int codec_set_property_internal(struct codec_channel_data *data, audio_property_t property,
				       audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_INPUT_VOLUME:
		if (val.vol > TAA3020_CHx_CFG2_DVOL_MAX || val.vol < TAA3020_CHx_CFG2_DVOL_MIN) {
			return -EINVAL;
		}
		data->volume = val.vol;
		break;
	case AUDIO_PROPERTY_INPUT_MUTE:
		data->muted = val.mute;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	int err;
	struct codec_driver_data *data = dev->data;

	for (size_t i = 0; i < ARRAY_SIZE(data->channels); i++) {
		if (!data->channels[i].configured) {
			continue;
		}
		if (data->channels[i].audio_channel != channel && channel != AUDIO_CHANNEL_ALL) {
			continue;
		}

		err = codec_set_property_internal(&data->channels[i], property, val);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int codec_apply_properties(const struct device *dev)
{
	struct codec_driver_data *data = dev->data;

	for (size_t i = 0; i < ARRAY_SIZE(data->channels); i++) {
		if (data->channels[i].configured) {
			codec_channel_set_volume(
				dev, i, (data->channels[i].muted) ? 0 : data->channels[i].volume);
		}
	}

	return 0;
}

static const struct audio_codec_api codec_driver_api = {
	.configure = codec_configure,
	.route_input = codec_route_input,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
};

static int codec_initialize_internal(const struct device *dev,
				     const struct codec_driver_config *const config)
{
	int err;
	uint8_t val;
	uint8_t in_ch_en = 0;

	codec_read_reg(dev, TAA3020_SLEEP_CFG, &val);
	val |= TAA3020_SLEEP_CFG_SLEEP_ENZ;

	if (config->areg_internal) {
		val |= TAA3020_SLEEP_CFG_AREG_SELECT;
	} else {
		val &= TAA3020_SLEEP_CFG_AREG_SELECT;
	}

	codec_write_reg(dev, TAA3020_SLEEP_CFG, val);
	k_msleep(1);

	for (size_t i = 0; i < config->num_channels; i++) {
		err = codec_configure_channel(dev, &config->channels[i]);
		if (err < 0) {
			LOG_ERR("Failed to configure channel #%zu: %d", i, err);
			return -EFAULT;
		}

		in_ch_en |= TAA3020_IN_CH_EN_CHANNEL(config->channels[i].channel);
	}

	in_ch_en &= TAA3020_IN_CH_EN_MASK;
	codec_write_reg(dev, TAA3020_IN_CH_EN, in_ch_en);

	return 0;
}

static int codec_initialize(const struct device *dev)
{
	int err;
	const struct codec_driver_config *const config = dev->config;

	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	codec_soft_reset(dev);
	err = codec_initialize_internal(dev, config);
	if (err < 0) {
		LOG_ERR("Failed to initialize codec: %d", err);
		return err;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void codec_enable_sleep(const struct device *dev)
{
	uint8_t val;

	codec_read_reg(dev, TAA3020_SLEEP_CFG, &val);
	val &= ~TAA3020_SLEEP_CFG_SLEEP_ENZ;
	codec_write_reg(dev, TAA3020_SLEEP_CFG, val);
	k_msleep(10);
}

static int codec_device_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;
	const struct codec_driver_config *const config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		err = codec_initialize_internal(dev, config);
		if (err < 0) {
			LOG_ERR("Failed to initialize codec: %d", err);
			return -EFAULT;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		codec_enable_sleep(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define TAA3020_CHAN_INIT(taa3020_channel)                                                         \
	{                                                                                          \
		.channel = DT_PROP_BY_IDX(taa3020_channel, reg, 0),                                \
		.in_type = DT_ENUM_IDX(taa3020_channel, in_type),                                  \
		.in_src = DT_ENUM_IDX(taa3020_channel, in_src),                                    \
		.coupling = DT_ENUM_IDX(taa3020_channel, coupling),                                \
		.impedance = DT_ENUM_IDX(taa3020_channel, impedance),                              \
		.agc_en = DT_PROP_OR(taa3020_channel, egc_en, false),                              \
	},

#define TAA3020_CHANNELS_INIT(idx) DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, TAA3020_CHAN_INIT)

#define TAA3020_INIT(n)                                                                            \
	PM_DEVICE_DT_INST_DEFINE(n, codec_device_pm_action);                                       \
                                                                                                   \
	static const struct codec_channel_config codec_channel_config_##n[] = {                    \
		TAA3020_CHANNELS_INIT(n)};                                                         \
	BUILD_ASSERT(ARRAY_SIZE(codec_channel_config_##n) <= 4, "TAA3020 supports max 4 channel"); \
                                                                                                   \
	static const struct codec_driver_config codec_device_config_##n = {                        \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.channels = codec_channel_config_##n,                                              \
		.num_channels = ARRAY_SIZE(codec_channel_config_##n),                              \
		.areg_internal = DT_INST_PROP_OR(n, areg_internal_en, false),                      \
	};                                                                                         \
	static struct codec_driver_data codec_driver_data_##n;                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, codec_initialize, PM_DEVICE_DT_INST_GET(n),                       \
			      &codec_driver_data_##n, &codec_device_config_##n, POST_KERNEL,       \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TAA3020_INIT)
