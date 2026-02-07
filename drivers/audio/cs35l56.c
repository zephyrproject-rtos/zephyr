/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/regulator.h>
#if CONFIG_AUDIO_CODEC_CS35L56_I2C
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#endif /* CONFIG_AUDIO_CODEC_CS35L56_I2C */

#include "cs35l56.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cirrus_cs35l56, CONFIG_AUDIO_CODEC_LOG_LEVEL);

union cs35l56_bus {
#if CONFIG_AUDIO_CODEC_CS35L56_I2C
	struct i2c_dt_spec i2c;
#endif /* CONFIG_AUDIO_CODEC_CS35L56_I2C */
};

typedef bool (*cs35l56_bus_is_ready_fn)(const union cs35l56_bus *bus);

struct cs35l56_config {
	struct gpio_dt_spec reset_gpio;
	const struct device *vdd_amp;
	const struct device *vdd_b;
	const struct device *vdd_a;
	const struct device *vdd_p;
	const union cs35l56_bus bus;
	cs35l56_bus_is_ready_fn bus_is_ready;
	uint32_t device_id;
};

static const uint32_t cs35l56_bclk_freq_hz[] = {
	[0xc] = 128000,    [0xf] = 256000,    [0x11] = 384000,   [0x12] = 512000,
	[0x15] = 768000,   [0x17] = 1024000,  [0x19] = 1411200,  [0x1a] = 1500000,
	[0x1b] = 1536000,  [0x1c] = 2000000,  [0x1d] = 2048000,  [0x1e] = 2400000,
	[0x1f] = 2822400,  [0x20] = 3000000,  [0x21] = 3072000,  [0x23] = 4000000,
	[0x24] = 4096000,  [0x25] = 4800000,  [0x26] = 5644800,  [0x27] = 6000000,
	[0x28] = 6144000,  [0x29] = 6250000,  [0x2a] = 6400000,  [0x2d] = 7526400,
	[0x2e] = 8000000,  [0x2f] = 8192000,  [0x30] = 9600000,  [0x31] = 11289600,
	[0x32] = 12000000, [0x33] = 12288000, [0x37] = 13500000, [0x38] = 19200000,
	[0x39] = 22579200, [0x3b] = 24576000,
};

#if CONFIG_AUDIO_CODEC_CS35L56_I2C
static int cs35l56_reg_read(const struct device *dev, const uint32_t reg_addr, uint32_t *const val)
{
	uint8_t read_buf[sizeof(*val)], write_buf[sizeof(reg_addr)];
	const struct cs35l56_config *config = dev->config;
	int ret;

	sys_put_be32(reg_addr, write_buf);

	ret = i2c_write_read_dt(&config->bus.i2c, write_buf, sizeof(reg_addr), read_buf,
				sizeof(*val));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_be32(read_buf);

	return 0;
}

static int cs35l56_reg_write(const struct device *dev, const uint32_t reg_addr, const uint32_t val)
{
	const struct cs35l56_config *config = dev->config;
	uint64_t msg = ((uint64_t)reg_addr << 32) | val;
	uint8_t buf[sizeof(msg)];

	sys_put_be64(msg, buf);

	return i2c_write_dt(&config->bus.i2c, buf, sizeof(msg));
}

static bool cs35l56_bus_is_ready_i2c(const union cs35l56_bus *bus)
{
	return device_is_ready(bus->i2c.bus);
}

__maybe_unused static int cs35l56_burst_write(const struct device *dev, const uint32_t reg_addr,
					      const uint8_t *buf, const uint32_t num_bytes)
{
	const struct cs35l56_config *config = dev->config;
	uint8_t addr_buf[sizeof(reg_addr)];
	struct i2c_msg msg[2];

	sys_put_be32(reg_addr, addr_buf);

	msg[0].buf = addr_buf;
	msg[0].len = ARRAY_SIZE(addr_buf);
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer_dt(&config->bus.i2c, msg, ARRAY_SIZE(msg));
}
#endif /* CONFIG_AUDIO_CODEC_CS35L56_I2C */

static int cs35l56_reg_update(const struct device *dev, uint32_t reg_addr, uint32_t mask,
			      uint32_t val)
{
	uint32_t orig, tmp;
	int ret;

	ret = cs35l56_reg_read(dev, reg_addr, &orig);
	if (ret < 0) {
		return ret;
	}

	tmp = orig & ~mask;
	tmp |= val & mask;

	return cs35l56_reg_write(dev, reg_addr, tmp);
}

static int cs35l56_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	int ret;

	switch (input) {
	case CS35L56_ASP1_TX1:
		ret = cs35l56_reg_update(dev, CS35L56_ASP1_FRAME_CONTROL1, CS35L56_ASP1_TX1_SLOT,
					 channel);
		if (ret < 0) {
			return ret;
		}

		ret = cs35l56_reg_update(dev, CS35L56_ASP1_ENABLES1, CS35L56_ASP1_TX1_EN,
					 CS35L56_ASP1_TX1_EN);
		if (ret < 0) {
			return ret;
		}
		break;
	case CS35L56_ASP1_TX2:
		ret = cs35l56_reg_update(dev, CS35L56_ASP1_FRAME_CONTROL1, CS35L56_ASP1_TX2_SLOT,
					 channel << CS35L56_ASP1_TX2_SHIFT);
		if (ret < 0) {
			return ret;
		}

		ret = cs35l56_reg_update(dev, CS35L56_ASP1_ENABLES1, CS35L56_ASP1_TX2_EN,
					 CS35L56_ASP1_TX2_EN);
		if (ret < 0) {
			return ret;
		}
		break;
	case CS35L56_ASP1_TX3:
		ret = cs35l56_reg_update(dev, CS35L56_ASP1_FRAME_CONTROL1, CS35L56_ASP1_TX3_SLOT,
					 channel << CS35L56_ASP1_TX3_SHIFT);
		if (ret < 0) {
			return ret;
		}

		ret = cs35l56_reg_update(dev, CS35L56_ASP1_ENABLES1, CS35L56_ASP1_TX3_EN,
					 CS35L56_ASP1_TX3_EN);
		if (ret < 0) {
			return ret;
		}
		break;
	case CS35L56_ASP1_TX4:
		ret = cs35l56_reg_update(dev, CS35L56_ASP1_FRAME_CONTROL1, CS35L56_ASP1_TX4_SLOT,
					 channel << CS35L56_ASP1_TX4_SHIFT);
		if (ret < 0) {
			return ret;
		}

		ret = cs35l56_reg_update(dev, CS35L56_ASP1_ENABLES1, CS35L56_ASP1_TX4_EN,
					 CS35L56_ASP1_TX4_EN);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int cs35l56_route_output(const struct device *dev, audio_channel_t channel, uint32_t output)
{
	int ret;

	switch (output) {
	case CS35L56_ASP1_RX1:
		ret = cs35l56_reg_update(dev, CS35L56_ASP1_FRAME_CONTROL5, CS35L56_ASP1_RX1_SLOT,
					 channel);
		if (ret < 0) {
			return ret;
		}

		ret = cs35l56_reg_update(dev, CS35L56_ASP1_ENABLES1, CS35L56_ASP1_RX1_EN,
					 CS35L56_ASP1_RX1_EN);
		if (ret < 0) {
			return ret;
		}
		break;
	case CS35L56_ASP1_RX2:
		ret = cs35l56_reg_update(dev, CS35L56_ASP1_FRAME_CONTROL5, CS35L56_ASP1_RX2_SLOT,
					 channel << CS35L56_ASP1_RX2_SHIFT);
		if (ret < 0) {
			return ret;
		}

		ret = cs35l56_reg_update(dev, CS35L56_ASP1_ENABLES1, CS35L56_ASP1_RX2_EN,
					 CS35L56_ASP1_RX2_EN);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int cs35l56_apply_properties(const struct device *dev)
{
	return 0;
}

static int cs35l56_output_volume(const struct device *dev, audio_channel_t channel,
				 audio_property_value_t audio_val)
{
	if (channel != AUDIO_CHANNEL_ALL) {
		return -EINVAL;
	}

	return cs35l56_reg_write(dev, CS35L56_MAIN_RENDER_USER_VOLUME, (uint32_t)audio_val.vol);
}

static int cs35l56_output_mute(const struct device *dev, audio_channel_t channel,
			       audio_property_value_t audio_val)
{
	if (channel != AUDIO_CHANNEL_ALL) {
		return -EINVAL;
	}

	return cs35l56_reg_write(dev, CS35L56_MAIN_RENDER_USER_MUTE, (uint32_t)audio_val.mute);
}

static int cs35l56_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return cs35l56_output_mute(dev, channel, val);
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return cs35l56_output_volume(dev, channel, val);
	default:
		return -ENOTSUP;
	}
}

static void cs35l56_stop_output(const struct device *dev)
{
	(void)cs35l56_reg_write(dev, CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_PAUSE);
}

static void cs35l56_start_output(const struct device *dev)
{
	(void)cs35l56_reg_write(dev, CS35L56_DSP_VIRTUAL1_MBOX_1, CS35L56_DSP_MBOX_CMD_PLAY);
}

static int cs35l56_asp1_set_clks(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct i2s_config i2s = cfg->dai_cfg.i2s;
	uint8_t asp1_bclk_freq = 0, clk_opt = 0;
	uint32_t bclk_freq_hz;
	int ret;

	if (i2s.frame_clk_freq != AUDIO_PCM_RATE_48K) {
		return -EINVAL;
	}

	if (i2s.word_size == AUDIO_PCM_WIDTH_16_BITS) {
		bclk_freq_hz = AUDIO_PCM_RATE_48K * i2s.channels * i2s.word_size;
	} else {
		bclk_freq_hz = AUDIO_PCM_RATE_48K * i2s.channels * AUDIO_PCM_WIDTH_32_BITS;
	}

	for (int i = CS35L56_ASP_BCLK_FREQ_OFFSET; i < ARRAY_SIZE(cs35l56_bclk_freq_hz); i++) {
		if (cs35l56_bclk_freq_hz[i] == bclk_freq_hz) {
			asp1_bclk_freq = i;
			break;
		}
	}

	if (asp1_bclk_freq == 0) {
		return -EINVAL;
	}

	ret = cs35l56_reg_write(dev, CS35L56_ASP1_CONTROL1, asp1_bclk_freq);
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(I2S_FMT_BIT_CLK_INV, i2s.format) > 0) {
		clk_opt |= CS35L56_ASP1_BCLK_INV;
	}

	if (FIELD_GET(I2S_FMT_FRAME_CLK_INV, i2s.format) > 0) {
		clk_opt |= CS35L56_ASP1_FSYNC_INV;
	}

	return cs35l56_reg_update(dev, CS35L56_ASP1_CONTROL2, CS35L56_BCLK_FSYNC_MASK, clk_opt);
}

static int cs35l56_asp1_set_word(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct i2s_config i2s = cfg->dai_cfg.i2s;
	uint8_t asp1_width;
	uint32_t val = 0;
	int ret;

	if (!IN_RANGE(i2s.word_size, CS35L56_ASP1_WL_MIN, CS35L56_ASP1_WL_MAX)) {
		return -EINVAL;
	}

	if (i2s.word_size == AUDIO_PCM_WIDTH_16_BITS) {
		asp1_width = AUDIO_PCM_WIDTH_16_BITS;
	} else {
		asp1_width = AUDIO_PCM_WIDTH_32_BITS;
	}

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		ret = cs35l56_reg_write(dev, CS35L56_ASP1_DATA_CONTROL1, i2s.word_size);
		if (ret < 0) {
			return ret;
		}

		val |= FIELD_PREP(CS35L56_ASP1_TX_WIDTH, asp1_width);
		__fallthrough;
	case AUDIO_ROUTE_PLAYBACK:
		ret = cs35l56_reg_write(dev, CS35L56_ASP1_DATA_CONTROL5, i2s.word_size);
		if (ret < 0) {
			return ret;
		}

		val |= FIELD_PREP(CS35L56_ASP1_RX_WIDTH, asp1_width);
		break;
	default:
		return -EINVAL;
	}

	switch (FIELD_GET(I2S_FMT_DATA_FORMAT_MASK, i2s.format)) {
	case I2S_FMT_DATA_FORMAT_I2S:
		val |= FIELD_PREP(CS35L56_ASP1_FMT_MASK, CS35L56_ASP1_FMT_I2S);
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		val |= FIELD_PREP(CS35L56_ASP1_FMT_MASK, CS35L56_ASP1_FMT_TDM15);
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		val |= FIELD_PREP(CS35L56_ASP1_FMT_MASK, CS35L56_ASP1_FMT_DSPA);
		break;
	default:
		return -ENOTSUP;
	}

	return cs35l56_reg_update(dev, CS35L56_ASP1_CONTROL2,
				  (CS35L56_ASP1_FMT_MASK | CS35L56_ASP1_WIDTH), val);
}

static int cs35l56_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	int ret;

	ret = cs35l56_asp1_set_clks(dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to set clocks");
		return ret;
	}

	ret = cs35l56_asp1_set_word(dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to set word");
		return ret;
	}

	return 0;
}

static int cs35l56_wait_for_rom_boot(const struct device *dev)
{
	const k_timepoint_t end = sys_timepoint_calc(CS35L56_ROM_BOOT_TIMEOUT);
	uint32_t val = 0;
	int ret;

	do {
		ret = cs35l56_reg_read(dev, CS35L56_HALO_STATE, &val);
		if (ret < 0) {
			return ret;
		}

		if (val == CS35L56_DSP_STATE_RUNNING) {
			return 0;
		}

		(void)k_sleep(CS35L56_ROM_BOOT_POLL_US);
	} while (!sys_timepoint_expired(end));

	return -EPERM;
}

static int cs35l56_reset(const struct device *dev)
{
	const struct cs35l56_config *config = dev->config;
	int ret;

	if (config->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		(void)k_sleep(CS35L56_T_RLPW_US);

		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}

		(void)k_sleep(CS35L56_T_IRS_US);
	} else {
		/*
		 * Note that the DSP firmware memory (RAM) contents are retained through software
		 * reset conditions.
		 */
		ret = cs35l56_reg_write(dev, CS35L56_SW_RESET_SFT_RESET_REG, 0x5A000000);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cs35l56_check_ids(const struct device *dev)
{
	const struct cs35l56_config *config = dev->config;
	uint32_t val;
	int ret;

	ret = cs35l56_reg_read(dev, CS35L56_SW_RESET_DEVID_REG, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != config->device_id) {
		return -EINVAL;
	}

	ret = cs35l56_reg_read(dev, CS35L56_SW_RESET_REVID_REG, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != 0xB2) {
		return -EINVAL;
	}

	return ret;
}

static int cs35l56_init_regulators(const struct device *dev)
{
	const struct cs35l56_config *config = dev->config;
	int ret;

	ret = regulator_enable(config->vdd_p);
	if (ret < 0) {
		return ret;
	}

	if (config->vdd_a != NULL) {
		ret = regulator_enable(config->vdd_a);
		if (ret < 0) {
			return ret;
		}
	}

	if (config->vdd_b != NULL) {
		ret = regulator_enable(config->vdd_b);
		if (ret < 0) {
			return ret;
		}
	} else if (config->vdd_amp != NULL) {
		ret = regulator_enable(config->vdd_amp);
		if (ret < 0) {
			return ret;
		}
	} else {
		LOG_DBG("Neither VDD AMP nor VDD B regulator provided");
		return -EINVAL;
	}

	return 0;
}

static int cs35l56_init(const struct device *dev)
{
	int ret;

	ret = cs35l56_init_regulators(dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable regulators: %d", ret);
		return ret;
	}

	ret = cs35l56_reset(dev);
	if (ret < 0) {
		LOG_ERR("Fail to reset: %d", ret);
		return ret;
	}

	ret = cs35l56_wait_for_rom_boot(dev);
	if (ret < 0) {
		LOG_ERR("Failed to boot from ROM: %d", ret);
		return ret;
	}

	ret = cs35l56_check_ids(dev);
	if (ret < 0) {
		LOG_ERR("Failed to check IDs: %d", ret);
		return ret;
	}

	return cs35l56_reg_update(dev, CS35L56_BLOCK_ENABLES2, CS35L56_ASP_EN, CS35L56_ASP_EN);
}

static const struct audio_codec_api api = {
	.configure = cs35l56_configure,
	.start_output = cs35l56_start_output,
	.stop_output = cs35l56_stop_output,
	.set_property = cs35l56_set_property,
	.apply_properties = cs35l56_apply_properties,
	.route_input = cs35l56_route_input,
	.route_output = cs35l56_route_output,
};

#define CS35L56_DEVICE_INIT(inst, name)                                                            \
	DEVICE_DT_INST_DEFINE(inst, cs35l56_init, NULL, NULL, &name##_config_##inst, POST_KERNEL,  \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &api);

#define CS35L56_CONFIG(inst, id)                                                                   \
	.vdd_amp = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(vdd_amp)),                                   \
	.vdd_b = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(vdd_b)),                                       \
	.vdd_p = DEVICE_DT_GET(DT_NODELABEL(vdd_p)),                                               \
	.vdd_a = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(vdd_a)),                                       \
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}), .device_id = id

#define CS35L56_CONFIG_I2C(inst, id)                                                               \
	{.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)},                                               \
	 .bus_is_ready = cs35l56_bus_is_ready_i2c,                                                 \
	 CS35L56_CONFIG(inst, id)}

#define CS35L56_DEFINE_I2C(inst, name, id)                                                         \
	static struct cs35l56_config name##_config_##inst = CS35L56_CONFIG_I2C(inst, id);          \
	CS35L56_DEVICE_INIT(inst, name)

#define AUDIO_CODEC_CS35L56_DEFINE(inst, name, id) CS35L56_DEFINE_I2C(inst, name, id)

#define DT_DRV_COMPAT cirrus_cs35l56
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define CS35L56_NAME      cs35l56
#define CS35L56_DEVICE_ID 0x35A56
DT_INST_FOREACH_STATUS_OKAY_VARGS(AUDIO_CODEC_CS35L56_DEFINE, CS35L56_NAME, CS35L56_DEVICE_ID)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT cirrus_cs35l57
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define CS35L57_NAME      cs35l57
#define CS35L57_DEVICE_ID 0x35A57
DT_INST_FOREACH_STATUS_OKAY_VARGS(AUDIO_CODEC_CS35L56_DEFINE, CS35L57_NAME, CS35L57_DEVICE_ID)
#endif
#undef DT_DRV_COMPAT
