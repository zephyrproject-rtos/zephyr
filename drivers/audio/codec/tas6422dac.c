/*
 * Copyright (c) 2023 Centralp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tas6422dac

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/audio/codec.h>

#include "tas6422dac.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tas6422dac);

#define TAS6422DAC_MUTE_GPIO_SUPPORT DT_ANY_INST_HAS_PROP_STATUS_OKAY(mute_gpios)

#define CODEC_OUTPUT_VOLUME_MAX (24 * 2)
#define CODEC_OUTPUT_VOLUME_MIN (-100 * 2)

struct codec_driver_config {
	struct i2c_dt_spec bus;
#if TAS6422DAC_MUTE_GPIO_SUPPORT
	struct gpio_dt_spec mute_gpio;
#endif /* TAS6422DAC_MUTE_GPIO_SUPPORT */
};

struct codec_driver_data {
};

enum tas6422dac_channel_t {
	TAS6422DAC_CHANNEL_1,
	TAS6422DAC_CHANNEL_2,
	TAS6422DAC_CHANNEL_ALL,
	TAS6422DAC_CHANNEL_UNKNOWN,
};

static enum tas6422dac_channel_t audio_to_tas6422dac_channel[] = {
	[AUDIO_CHANNEL_FRONT_LEFT] = TAS6422DAC_CHANNEL_1,
	[AUDIO_CHANNEL_FRONT_RIGHT] = TAS6422DAC_CHANNEL_2,
	[AUDIO_CHANNEL_LFE] = TAS6422DAC_CHANNEL_UNKNOWN,
	[AUDIO_CHANNEL_FRONT_CENTER] = TAS6422DAC_CHANNEL_UNKNOWN,
	[AUDIO_CHANNEL_REAR_LEFT] = TAS6422DAC_CHANNEL_1,
	[AUDIO_CHANNEL_REAR_RIGHT] = TAS6422DAC_CHANNEL_2,
	[AUDIO_CHANNEL_REAR_CENTER] = TAS6422DAC_CHANNEL_UNKNOWN,
	[AUDIO_CHANNEL_SIDE_LEFT] = TAS6422DAC_CHANNEL_1,
	[AUDIO_CHANNEL_SIDE_RIGHT] = TAS6422DAC_CHANNEL_2,
	[AUDIO_CHANNEL_ALL] = TAS6422DAC_CHANNEL_ALL,
};

static void codec_mute_output(const struct device *dev, enum tas6422dac_channel_t channel);
static void codec_unmute_output(const struct device *dev, enum tas6422dac_channel_t channel);
static void codec_write_reg(const struct device *dev, uint8_t reg, uint8_t val);
static void codec_read_reg(const struct device *dev, uint8_t reg, uint8_t *val);
static void codec_soft_reset(const struct device *dev);
static int codec_configure_dai(const struct device *dev, audio_dai_cfg_t *cfg);
static void codec_configure_output(const struct device *dev);
static int codec_set_output_volume(const struct device *dev, enum tas6422dac_channel_t channel,
				   int vol);

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
static void codec_read_all_regs(const struct device *dev);
#define CODEC_DUMP_REGS(dev) codec_read_all_regs((dev))
#else
#define CODEC_DUMP_REGS(dev)
#endif

static int codec_initialize(const struct device *dev)
{
	const struct codec_driver_config *const dev_cfg = dev->config;

	if (!device_is_ready(dev_cfg->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

#if TAS6422DAC_MUTE_GPIO_SUPPORT
	if (!device_is_ready(dev_cfg->mute_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}
#endif /* TAS6422DAC_MUTE_GPIO_SUPPORT */

	return 0;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	int ret;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("dai_type must be AUDIO_DAI_TYPE_I2S");
		return -EINVAL;
	}

	codec_soft_reset(dev);
	ret = codec_configure_dai(dev, &cfg->dai_cfg);
	codec_configure_output(dev);

	return ret;
}

static void codec_start_output(const struct device *dev)
{
	codec_unmute_output(dev, TAS6422DAC_CHANNEL_ALL);

	CODEC_DUMP_REGS(dev);
}

static void codec_stop_output(const struct device *dev)
{
	codec_mute_output(dev, TAS6422DAC_CHANNEL_ALL);
}

static void codec_mute_output(const struct device *dev, enum tas6422dac_channel_t channel)
{
	uint8_t val;

#if TAS6422DAC_MUTE_GPIO_SUPPORT
	const struct codec_driver_config *const dev_cfg = dev->config;

	if (channel == TAS6422DAC_CHANNEL_ALL) {
		gpio_pin_configure_dt(&dev_cfg->mute_gpio, GPIO_OUTPUT_ACTIVE);
	}
#endif

	codec_read_reg(dev, CH_STATE_CTRL_ADDR, &val);
	switch (channel) {
	case TAS6422DAC_CHANNEL_1:
		val &= ~CH_STATE_CTRL_CH1_STATE_CTRL_MASK;
		val |= CH_STATE_CTRL_CH1_STATE_CTRL(CH_STATE_CTRL_MUTE);
		break;
	case TAS6422DAC_CHANNEL_2:
		val &= ~CH_STATE_CTRL_CH2_STATE_CTRL_MASK;
		val |= CH_STATE_CTRL_CH2_STATE_CTRL(CH_STATE_CTRL_MUTE);
		break;
	case TAS6422DAC_CHANNEL_ALL:
		val &= ~(CH_STATE_CTRL_CH1_STATE_CTRL_MASK | CH_STATE_CTRL_CH2_STATE_CTRL_MASK);
		val |= CH_STATE_CTRL_CH1_STATE_CTRL(CH_STATE_CTRL_MUTE) |
		       CH_STATE_CTRL_CH2_STATE_CTRL(CH_STATE_CTRL_MUTE);
		break;
	case TAS6422DAC_CHANNEL_UNKNOWN:
	default:
		LOG_ERR("Invalid codec channel %u", channel);
		return;
	}
	codec_write_reg(dev, CH_STATE_CTRL_ADDR, val);
}

static void codec_unmute_output(const struct device *dev, enum tas6422dac_channel_t channel)
{
	uint8_t val;

#if TAS6422DAC_MUTE_GPIO_SUPPORT
	const struct codec_driver_config *const dev_cfg = dev->config;

	gpio_pin_configure_dt(&dev_cfg->mute_gpio, GPIO_OUTPUT_INACTIVE);
#endif

	codec_read_reg(dev, CH_STATE_CTRL_ADDR, &val);
	switch (channel) {
	case TAS6422DAC_CHANNEL_1:
		val &= ~CH_STATE_CTRL_CH1_STATE_CTRL_MASK;
		val |= CH_STATE_CTRL_CH1_STATE_CTRL(CH_STATE_CTRL_PLAY);
		break;
	case TAS6422DAC_CHANNEL_2:
		val &= ~CH_STATE_CTRL_CH2_STATE_CTRL_MASK;
		val |= CH_STATE_CTRL_CH2_STATE_CTRL(CH_STATE_CTRL_PLAY);
		break;
	case TAS6422DAC_CHANNEL_ALL:
		val &= ~(CH_STATE_CTRL_CH1_STATE_CTRL_MASK | CH_STATE_CTRL_CH2_STATE_CTRL_MASK);
		val |= CH_STATE_CTRL_CH1_STATE_CTRL(CH_STATE_CTRL_PLAY) |
		       CH_STATE_CTRL_CH2_STATE_CTRL(CH_STATE_CTRL_PLAY);
		break;
	case TAS6422DAC_CHANNEL_UNKNOWN:
	default:
		LOG_ERR("Invalid codec channel %u", channel);
		return;
	}
	codec_write_reg(dev, CH_STATE_CTRL_ADDR, val);
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	enum tas6422dac_channel_t codec_channel = audio_to_tas6422dac_channel[channel];

	if (codec_channel == TAS6422DAC_CHANNEL_UNKNOWN) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return codec_set_output_volume(dev, codec_channel, val.vol);

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (val.mute) {
			codec_mute_output(dev, codec_channel);
		} else {
			codec_unmute_output(dev, codec_channel);
		}
		return 0;

	default:
		break;
	}

	return -EINVAL;
}

static int codec_apply_properties(const struct device *dev)
{
	/* nothing to do because there is nothing cached */
	return 0;
}

static void codec_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct codec_driver_config *const dev_cfg = dev->config;

	i2c_reg_write_byte_dt(&dev_cfg->bus, reg, val);
	LOG_DBG("%s WR REG:0x%02x VAL:0x%02x", dev->name, reg, val);
}

static void codec_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct codec_driver_config *const dev_cfg = dev->config;

	i2c_reg_read_byte_dt(&dev_cfg->bus, reg, val);
	LOG_DBG("%s RD REG:0x%02x VAL:0x%02x", dev->name, reg, *val);
}

static void codec_soft_reset(const struct device *dev)
{
	uint8_t val;

	codec_read_reg(dev, MODE_CTRL_ADDR, &val);
	val |= MODE_CTRL_RESET;
	codec_write_reg(dev, MODE_CTRL_ADDR, val);
}

static int codec_configure_dai(const struct device *dev, audio_dai_cfg_t *cfg)
{
	uint8_t val;

	codec_read_reg(dev, SAP_CTRL_ADDR, &val);

	/* I2S mode */
	val &= ~SAP_CTRL_INPUT_FORMAT_MASK;
	val |= SAP_CTRL_INPUT_FORMAT(SAP_CTRL_INPUT_FORMAT_I2S);

	/* Input sampling rate */
	val &= ~SAP_CTRL_INPUT_SAMPLING_RATE_MASK;
	switch (cfg->i2s.frame_clk_freq) {
	case AUDIO_PCM_RATE_44P1K:
		val |= SAP_CTRL_INPUT_SAMPLING_RATE(SAP_CTRL_INPUT_SAMPLING_RATE_44_1_KHZ);
		break;
	case AUDIO_PCM_RATE_48K:
		val |= SAP_CTRL_INPUT_SAMPLING_RATE(SAP_CTRL_INPUT_SAMPLING_RATE_48_KHZ);
		break;
	case AUDIO_PCM_RATE_96K:
		val |= SAP_CTRL_INPUT_SAMPLING_RATE(SAP_CTRL_INPUT_SAMPLING_RATE_96_KHZ);
		break;
	default:
		LOG_ERR("Invalid sampling rate %zu", cfg->i2s.frame_clk_freq);
		return -EINVAL;
	}

	codec_write_reg(dev, SAP_CTRL_ADDR, val);

	return 0;
}

static void codec_configure_output(const struct device *dev)
{
	uint8_t val;

	/* Overcurrent level = 1 */
	codec_read_reg(dev, MISC_CTRL_1_ADDR, &val);
	val &= ~MISC_CTRL_1_OC_CONTROL_MASK;
	codec_write_reg(dev, MISC_CTRL_1_ADDR, val);

	/*
	 * PWM frequency = 10 fs
	 * Reduce PWM frequency to prevent component overtemperature
	 */
	codec_read_reg(dev, MISC_CTRL_2_ADDR, &val);
	val &= ~MISC_CTRL_2_PWM_FREQUENCY_MASK;
	val |= MISC_CTRL_2_PWM_FREQUENCY(MISC_CTRL_2_PWM_FREQUENCY_10_FS);
	codec_write_reg(dev, MISC_CTRL_2_ADDR, val);
}

static int codec_set_output_volume(const struct device *dev, enum tas6422dac_channel_t channel,
				   int vol)
{
	uint8_t vol_val;

	if ((vol > CODEC_OUTPUT_VOLUME_MAX) || (vol < CODEC_OUTPUT_VOLUME_MIN)) {
		LOG_ERR("Invalid volume %d.%d dB", vol >> 1, ((uint32_t)vol & 1) ? 5 : 0);
		return -EINVAL;
	}

	vol_val = vol + 0xcf;
	switch (channel) {
	case TAS6422DAC_CHANNEL_1:
		codec_write_reg(dev, CH1_VOLUME_CTRL_ADDR, CH_VOLUME_CTRL_VOLUME(vol_val));
		break;
	case TAS6422DAC_CHANNEL_2:
		codec_write_reg(dev, CH2_VOLUME_CTRL_ADDR, CH_VOLUME_CTRL_VOLUME(vol_val));
		break;
	case TAS6422DAC_CHANNEL_ALL:
		codec_write_reg(dev, CH1_VOLUME_CTRL_ADDR, CH_VOLUME_CTRL_VOLUME(vol_val));
		codec_write_reg(dev, CH2_VOLUME_CTRL_ADDR, CH_VOLUME_CTRL_VOLUME(vol_val));
		break;
	case TAS6422DAC_CHANNEL_UNKNOWN:
	default:
		LOG_ERR("Invalid codec channel %u", channel);
		return -EINVAL;
	}

	return 0;
}

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
static void codec_read_all_regs(const struct device *dev)
{
	uint8_t val;

	codec_read_reg(dev, MODE_CTRL_ADDR, &val);
	codec_read_reg(dev, MISC_CTRL_1_ADDR, &val);
	codec_read_reg(dev, MISC_CTRL_2_ADDR, &val);
	codec_read_reg(dev, SAP_CTRL_ADDR, &val);
	codec_read_reg(dev, CH_STATE_CTRL_ADDR, &val);
	codec_read_reg(dev, CH1_VOLUME_CTRL_ADDR, &val);
	codec_read_reg(dev, CH2_VOLUME_CTRL_ADDR, &val);
	codec_read_reg(dev, DC_LDG_CTRL_1_ADDR, &val);
	codec_read_reg(dev, DC_LDG_CTRL_2_ADDR, &val);
	codec_read_reg(dev, DC_LDG_REPORT_1_ADDR, &val);
	codec_read_reg(dev, DC_LDG_REPORT_3_ADDR, &val);
	codec_read_reg(dev, CH_FAULTS_ADDR, &val);
	codec_read_reg(dev, GLOBAL_FAULTS_1_ADDR, &val);
	codec_read_reg(dev, GLOBAL_FAULTS_2_ADDR, &val);
	codec_read_reg(dev, WARNINGS_ADDR, &val);
	codec_read_reg(dev, PIN_CTRL_ADDR, &val);
	codec_read_reg(dev, MISC_CTRL_3_ADDR, &val);
	codec_read_reg(dev, ILIMIT_STATUS_ADDR, &val);
	codec_read_reg(dev, MISC_CTRL_4_ADDR, &val);
	codec_read_reg(dev, MISC_CTRL_5_ADDR, &val);
}
#endif

static const struct audio_codec_api codec_driver_api = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
};

#if TAS6422DAC_MUTE_GPIO_SUPPORT
#define TAS6422DAC_MUTE_GPIO_INIT(n) .mute_gpio = GPIO_DT_SPEC_INST_GET(n, mute_gpios)
#else
#define TAS6422DAC_MUTE_GPIO_INIT(n)
#endif /* TAS6422DAC_MUTE_GPIO_SUPPORT */

#define TAS6422DAC_INIT(n)                                                                         \
	static struct codec_driver_data codec_device_data_##n;                                     \
                                                                                                   \
	static struct codec_driver_config codec_device_config_##n = {                              \
		.bus = I2C_DT_SPEC_INST_GET(n), TAS6422DAC_MUTE_GPIO_INIT(n)};                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, codec_initialize, NULL, &codec_device_data_##n,                   \
			      &codec_device_config_##n, POST_KERNEL,                               \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TAS6422DAC_INIT)
