/*
 *  Copyright 2024 Cirrus Logic, Inc.
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirrus_cs35l45

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/regulator.h>
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY */

#include "cs35l45.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cirrus_cs35l45, CONFIG_AUDIO_CODEC_LOG_LEVEL);

union cs35l45_bus {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY */
};

typedef bool (*cs35l45_bus_is_ready_fn)(const union cs35l45_bus *bus);

struct cs35l45_config {
	struct gpio_dt_spec reset_gpio;
	const struct device *vdd_batt;
	const struct device *vdd_a;
	const union cs35l45_bus bus;
	cs35l45_bus_is_ready_fn bus_is_ready;
};

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
static int cs35l45_reg_read(const struct device *dev, uint32_t reg_addr, uint32_t *val)
{
	uint8_t read_buf[sizeof(uint32_t)], write_buf[sizeof(uint32_t)];
	const struct cs35l45_config *config = dev->config;
	int ret;

	sys_put_be32(reg_addr, write_buf);

	ret = i2c_write_read_dt(&config->bus.i2c, write_buf, sizeof(uint32_t), read_buf,
				sizeof(uint32_t));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be32(read_buf);

	return 0;
}

static int cs35l45_reg_write(const struct device *dev, uint32_t reg_addr, uint32_t val)
{
	const struct cs35l45_config *config = dev->config;
	uint64_t msg = ((uint64_t)reg_addr << 32) | val;
	uint8_t buf[sizeof(uint64_t)];

	sys_put_be64(msg, buf);

	return i2c_write_dt(&config->bus.i2c, buf, sizeof(uint64_t));
}

static bool cs35l45_bus_is_ready_i2c(const union cs35l45_bus *bus)
{
	return device_is_ready(bus->i2c.bus);
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY */

static int cs35l45_reg_update(const struct device *dev, uint32_t reg_addr, uint32_t mask,
			      uint32_t val)
{
	uint32_t tmp, orig;
	int ret;

	ret = cs35l45_reg_read(dev, reg_addr, &orig);
	if (ret < 0) {
		return ret;
	}

	tmp = orig & ~mask;
	tmp |= val & mask;

	return cs35l45_reg_write(dev, reg_addr, tmp);
}

static int cs35l45_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	uint32_t val;
	int ret;

	switch (input) {
	case CS35L45_DACPCM1_SRC_ASP_RX1:
		val = CS35L45_ASP_RX1_EN;
		break;
	case CS35L45_DACPCM1_SRC_ASP_RX2:
		val = CS35L45_ASP_RX2_EN;
		break;
	default:
		return -EINVAL;
	}

	ret = cs35l45_reg_update(dev, CS35L45_ASP_ENABLES1, val, val);
	if (ret < 0) {
		return ret;
	}

	ret = cs35l45_reg_update(dev, CS35L45_BLOCK_ENABLES2, CS35L45_ASP_EN, CS35L45_ASP_EN);
	if (ret < 0) {
		return ret;
	}

	return cs35l45_reg_update(dev, CS35L45_DACPCM1_INPUT, CS35L45_DACPCM1_SRC_MASK, input);
}

static int cs35l45_apply_properties(const struct device *dev)
{
	return 0;
}

static int cs35l45_set_pcm_volume(const struct device *dev, int vol)
{
	if (!IN_RANGE(vol, CS35L45_AMP_VOL_PCM_MIN, CS35L45_AMP_VOL_PCM_MAX)) {
		return -EINVAL;
	}

	return cs35l45_reg_update(dev, CS35L45_AMP_PCM_CONTROL, CS35L45_AMP_VOL_PCM_MASK, vol);
}

static int cs35l45_set_mute(const struct device *dev, const bool mute)
{
	uint32_t val, hpf_tune;
	uint8_t global_fs;
	int ret;

	if (!mute) {
		ret = cs35l45_reg_read(dev, CS35L45_GLOBAL_SAMPLE_RATE, &val);
		if (ret < 0) {
			return ret;
		}

		global_fs = FIELD_GET(CS35L45_GLOBAL_FS_MASK, val);

		switch (global_fs) {
		case CS35L45_GLOBAL_FS_44P1K:
			hpf_tune = CS35L45_HPF_44P1;
			break;
		default:
			hpf_tune = CS35L45_HPF_DEFAULT;
			break;
		}

		ret = cs35l45_reg_read(dev, CS35L45_AMP_PCM_HPF_TST, &val);
		if (ret < 0) {
			return ret;
		}

		if (val != hpf_tune) {
			struct reg_sequence hpf_override_seq[] = {
				{0x00000040, 0x00000055},
				{0x00000040, 0x000000AA},
				{0x00000044, 0x00000055},
				{0x00000044, 0x000000AA},
				{CS35L45_AMP_PCM_HPF_TST, hpf_tune},
				{0x00000040, 0x00000000},
				{0x00000044, 0x00000000},
			};

			for (int i = 0; i < ARRAY_SIZE(hpf_override_seq); i++) {
				ret = cs35l45_reg_write(dev, hpf_override_seq[i].reg,
							hpf_override_seq[i].def);
				if (ret < 0) {
					return ret;
				}
			}
		}
	}

	return cs35l45_reg_update(dev, CS35L45_AMP_OUTPUT_MUTE, CS35L45_AMP_MUTE,
				  FIELD_PREP(CS35L45_AMP_MUTE, mute));
}

static int cs35l45_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_INPUT_MUTE:
		return cs35l45_set_mute(dev, val.mute);
	case AUDIO_PROPERTY_INPUT_VOLUME:
		return cs35l45_set_pcm_volume(dev, val.vol);
	default:
		return -EINVAL;
	}
}

static int cs35l45_global_en_event(const struct device *dev, const bool enable)
{
	int ret;

	if (enable) {
		ret = cs35l45_reg_write(dev, CS35L45_GLOBAL_ENABLES, CS35L45_GLOBAL_EN_MASK);
		if (ret < 0) {
			return ret;
		}

		k_usleep(CS35L45_POST_GLOBAL_EN_US);
	} else {
		k_usleep(CS35L45_PRE_GLOBAL_DIS_US);
		ret = cs35l45_reg_write(dev, CS35L45_GLOBAL_ENABLES, 0);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static void cs35l45_stop_output(const struct device *dev)
{
	cs35l45_global_en_event(dev, false);
}

static void cs35l45_start_output(const struct device *dev)
{
	cs35l45_global_en_event(dev, true);
}

static int cs35l45_get_clk_freq_id(const uint32_t freq)
{
	int i;

	if (freq == 0) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(cs35l45_pll_refclk_freq); ++i) {
		if (cs35l45_pll_refclk_freq[i].freq == freq) {
			return cs35l45_pll_refclk_freq[i].cfg_id;
		}
	}

	return -EINVAL;
}

static int cs35l45_set_pll(const struct device *dev, const uint32_t freq)
{
	uint8_t freq_id;
	uint32_t val;
	int ret;

	freq_id = cs35l45_get_clk_freq_id(freq);
	if (freq_id < 0) {
		LOG_DBG("Invalid freq: %u", freq);
		return -EINVAL;
	}

	ret = cs35l45_reg_read(dev, CS35L45_REFCLK_INPUT, &val);
	if (ret < 0) {
		return ret;
	}

	val = FIELD_GET(CS35L45_PLL_REFCLK_FREQ_MASK, val);
	if (val == freq_id) {
		return 0;
	}

	ret = cs35l45_reg_update(dev, CS35L45_REFCLK_INPUT, CS35L45_PLL_OPEN_LOOP_MASK,
				 CS35L45_PLL_OPEN_LOOP_MASK);
	if (ret < 0) {
		return ret;
	}

	ret = cs35l45_reg_update(dev, CS35L45_REFCLK_INPUT, CS35L45_PLL_REFCLK_FREQ_MASK,
				 FIELD_PREP(CS35L45_PLL_REFCLK_FREQ_MASK, freq_id));
	if (ret < 0) {
		return ret;
	}

	ret = cs35l45_reg_update(dev, CS35L45_REFCLK_INPUT, CS35L45_PLL_REFCLK_EN_MASK, 0);
	if (ret < 0) {
		return ret;
	}

	ret = cs35l45_reg_update(dev, CS35L45_REFCLK_INPUT, CS35L45_PLL_OPEN_LOOP_MASK, 0);
	if (ret < 0) {
		return ret;
	}

	return cs35l45_reg_update(dev, CS35L45_REFCLK_INPUT, CS35L45_PLL_REFCLK_EN_MASK,
				  CS35L45_PLL_REFCLK_EN_MASK);
}

static int cs35l45_set_frame_clock(const struct device *dev, const uint32_t freq)
{
	uint8_t global_fs;

	switch (freq) {
	case AUDIO_PCM_RATE_44P1K:
		global_fs = CS35L45_GLOBAL_FS_44P1K;
		break;
	case AUDIO_PCM_RATE_48K:
		global_fs = CS35L45_GLOBAL_FS_48K;
		break;
	case AUDIO_PCM_RATE_96K:
		global_fs = CS35L45_GLOBAL_FS_96K;
		break;
	default:
		LOG_DBG("Unsupported frame clock frequency: %d Hz", freq);
		return -EINVAL;
	}

	return cs35l45_reg_update(dev, CS35L45_GLOBAL_SAMPLE_RATE, CS35L45_GLOBAL_FS_MASK,
				  global_fs);
}

static int cs35l45_configure_asp_fmt(const struct device *dev, const i2s_fmt_t i2s_fmt,
				     const i2s_opt_t i2s_opt)
{
	uint8_t asp_fmt;
	uint32_t val;

	if ((i2s_opt & (I2S_OPT_BIT_CLK_SLAVE | I2S_OPT_FRAME_CLK_SLAVE)) == 0U) {
		LOG_DBG("Invalid DAI clocking");
		return -EINVAL;
	}

	switch (FIELD_GET(I2S_FMT_DATA_FORMAT_MASK, i2s_fmt)) {
	case I2S_FMT_DATA_FORMAT_I2S:
		asp_fmt = CS35L45_ASP_FMT_I2S;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		asp_fmt = CS35L45_ASP_FMT_TDM_1_5;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		asp_fmt = CS35L45_ASP_FMT_DSP_A;
		break;
	default:
		LOG_DBG("Invalid DAI format");
		return -EINVAL;
	}

	val = FIELD_PREP(CS35L45_ASP_FMT_MASK, asp_fmt);

	switch (FIELD_GET(I2S_FMT_CLK_FORMAT_SHIFT, i2s_fmt)) {
	case I2S_FMT_CLK_NF_NB:
		break;
	case I2S_FMT_CLK_NF_IB:
		val |= CS35L45_ASP_BCLK_INV;
		break;
	case I2S_FMT_CLK_IF_NB:
		val |= CS35L45_ASP_FSYNC_INV;
		break;
	case I2S_FMT_CLK_IF_IB:
		val |= (CS35L45_ASP_FSYNC_INV | CS35L45_ASP_BCLK_INV);
		break;
	default:
		LOG_DBG("Invalid DAI clock polarity");
	}

	return cs35l45_reg_update(
		dev, CS35L45_ASP_CONTROL2,
		(CS35L45_ASP_FMT_MASK | CS35L45_ASP_BCLK_INV | CS35L45_ASP_FSYNC_INV), val);
}

static int cs35l45_configure_asp_word(const struct device *dev, const uint8_t word_size,
				      const uint8_t channels, const audio_route_t dai_route)
{
	uint8_t asp_width;
	int ret;

	if (!IN_RANGE(word_size, CS35L45_ASP_WL_MIN, CS35L45_ASP_WL_MAX)) {
		return -EINVAL;
	}

	asp_width = word_size * channels;

	if (!IN_RANGE(asp_width, CS35L45_ASP_WIDTH_MIN, CS35L45_ASP_WIDTH_MAX)) {
		return -EINVAL;
	}

	if (dai_route == AUDIO_ROUTE_PLAYBACK) {
		ret = cs35l45_reg_update(dev, CS35L45_ASP_CONTROL2, CS35L45_ASP_WIDTH_RX_MASK,
					 FIELD_PREP(CS35L45_ASP_WIDTH_RX_MASK, asp_width));
		if (ret < 0) {
			return ret;
		}

		return cs35l45_reg_update(dev, CS35L45_ASP_DATA_CONTROL5, CS35L45_ASP_WL_MASK,
					  word_size);

	} else if (dai_route == AUDIO_ROUTE_CAPTURE) {
		ret = cs35l45_reg_update(dev, CS35L45_ASP_CONTROL2, CS35L45_ASP_WIDTH_TX_MASK,
					 FIELD_PREP(CS35L45_ASP_WIDTH_TX_MASK, asp_width));
		if (ret < 0) {
			return ret;
		}

		return cs35l45_reg_update(dev, CS35L45_ASP_DATA_CONTROL1, CS35L45_ASP_WL_MASK,
					  asp_width);

	} else {
		return -EINVAL;
	}
}

static int cs35l45_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	uint32_t bclk_freq;
	int ret;

	ret = cs35l45_set_frame_clock(dev, cfg->dai_cfg.i2s.frame_clk_freq);
	if (ret < 0) {
		return ret;
	}

	ret = cs35l45_configure_asp_word(dev, cfg->dai_cfg.i2s.word_size, cfg->dai_cfg.i2s.channels,
					 cfg->dai_route);
	if (ret < 0) {
		return ret;
	}

	ret = cs35l45_configure_asp_fmt(dev, cfg->dai_cfg.i2s.format, cfg->dai_cfg.i2s.options);
	if (ret < 0) {
		return ret;
	}

	if (cfg->dai_cfg.i2s.format == I2S_FMT_DATA_FORMAT_I2S) {
		bclk_freq = cfg->dai_cfg.i2s.frame_clk_freq * cfg->dai_cfg.i2s.word_size *
			    I2S_FMT_I2S_CHANNELS * 2;
	} else {
		bclk_freq = cfg->dai_cfg.i2s.frame_clk_freq * cfg->dai_cfg.i2s.word_size *
			    cfg->dai_cfg.i2s.channels;
	}

	return cs35l45_set_pll(dev, bclk_freq);
}

static int cs35l45_apply_patch(const struct device *dev)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(cs35l45_patch); i++) {
		ret = cs35l45_reg_write(dev, cs35l45_patch[i].reg, cs35l45_patch[i].def);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cs35l45_hw_init(const struct device *dev)
{
	uint32_t val = 0;
	int ret, i = 5;

	while (!(val & CS35L45_OTP_BOOT_DONE_STS_MASK)) {
		k_usleep(1000);
		cs35l45_reg_read(dev, CS35L45_IRQ1_EINT_4, &val);
		i--;
		if (i < 0) {
			return -ETIMEDOUT;
		}
	}

	ret = cs35l45_reg_read(dev, CS35L45_DEVID, &val);
	if (ret < 0) {
		return ret;
	}

	if (val == 0x35A450) {
		LOG_INF("Found DEVID:0x%x", val);
	} else {
		LOG_DBG("Bad DEVID 0x%x", val);
		return -ENODEV;
	}

	ret = cs35l45_reg_write(dev, CS35L45_IRQ1_EINT_4,
				(CS35L45_OTP_BOOT_DONE_STS_MASK | CS35L45_OTP_BUSY_MASK));
	if (ret < 0) {
		return ret;
	}

	ret = cs35l45_apply_patch(dev);
	if (ret < 0) {
		return ret;
	}

	return cs35l45_reg_update(dev, CS35L45_BLOCK_ENABLES, CS35L45_BST_EN_MASK,
				  FIELD_PREP(CS35L45_BST_DISABLE_FET_ON, CS35L45_BST_EN_MASK));
}

static int cs35l45_init(const struct device *dev)
{
	const struct cs35l45_config *config = dev->config;
	int ret;

	if (!config->bus_is_ready) {
		return -ENODEV;
	}

	if (config->vdd_batt != NULL) {
		ret = regulator_enable(config->vdd_batt);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->vdd_a != NULL) {
		ret = regulator_enable(config->vdd_a);
		if (ret < 0) {
			return ret;
		}
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret == -EBUSY) {
		LOG_DBG("Reset line is busy, assuming shared reset");
	} else if (ret < 0) {
		return ret;
	}

	k_usleep(CS35L45_T_RLPW_US);
	gpio_pin_set_dt(&config->reset_gpio, 0);
	k_usleep(CS35L45_T_IRS_US);

	return cs35l45_hw_init(dev);
}

static const struct audio_codec_api api = {
	.configure = cs35l45_configure,
	.start_output = cs35l45_start_output,
	.stop_output = cs35l45_stop_output,
	.set_property = cs35l45_set_property,
	.apply_properties = cs35l45_apply_properties,
	.route_input = cs35l45_route_input,
};

#define CS35L45_DEVICE_INIT(inst)                                                                  \
	DEVICE_DT_INST_DEFINE(inst, cs35l45_init, NULL, NULL, &cs35l45_config_##inst, POST_KERNEL, \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &api);

#define CS35L45_CONFIG(inst)                                                                       \
	.vdd_batt = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(vdd_batt)),                                 \
	.vdd_a = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(vdd_a)),                                       \
	.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),

#define CS35L45_CONFIG_I2C(inst)                                                                   \
	{.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)},                                               \
	 .bus_is_ready = cs35l45_bus_is_ready_i2c,                                                 \
	 CS35L45_CONFIG(inst)}

#define CS35L45_DEFINE_I2C(inst)                                                                   \
	static struct cs35l45_config cs35l45_config_##inst = CS35L45_CONFIG_I2C(inst);             \
	CS35L45_DEVICE_INIT(inst)

#define AUDIO_CODEC_CS35L45_DEFINE(inst) CS35L45_DEFINE_I2C(inst)

DT_INST_FOREACH_STATUS_OKAY(AUDIO_CODEC_CS35L45_DEFINE)
