/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/audio/cs35l5x.h>

#include "cs35lxx.h"

LOG_MODULE_REGISTER(cirrus_cs35l5x, CONFIG_AUDIO_CODEC_LOG_LEVEL);

/* Register map */
#define CS35L5X_SW_RESET_DEVID_REG     0x0
#define CS35L5X_SW_RESET_REVID_REG     0x4
#define CS35L5X_SW_RESET_SFT_RESET_REG 0x20

#define CS35L5X_DSP_VIRTUAL1_MBOX_1 0x11020

#define CS35L5X_HALO_STATE 0x2803D20

#define CS35L5X_BLOCK_ENABLES2 0x0201C
#define CS35L5X_ASP_EN         BIT(27)

#define CS35L5X_ASP1_ENABLES1 0x04800
#define CS35L5X_ASP1_RX2_EN   BIT(17)
#define CS35L5X_ASP1_RX1_EN   BIT(16)
#define CS35L5X_ASP1_TX4_EN   BIT(3)
#define CS35L5X_ASP1_TX3_EN   BIT(2)
#define CS35L5X_ASP1_TX2_EN   BIT(1)
#define CS35L5X_ASP1_TX1_EN   BIT(0)

#define CS35L5X_ASP1_CONTROL1        0x04804
#define CS35L5X_ASP_BCLK_FREQ_OFFSET 0xC

#define CS35L5X_ASP1_CONTROL2   0x04808
#define CS35L5X_ASP1_RX_WIDTH   GENMASK(31, 24)
#define CS35L5X_ASP1_TX_WIDTH   GENMASK(23, 16)
#define CS35L5X_ASP1_WIDTH      (CS35L5X_ASP1_RX_WIDTH | CS35L5X_ASP1_TX_WIDTH)
#define CS35L5X_ASP1_WIDTH_MAX  0x80
#define CS35L5X_ASP1_WIDTH_MIN  0xc
#define CS35L5X_ASP1_FMT_MASK   GENMASK(10, 8)
#define CS35L5X_ASP1_FMT_DSPA   0x0
#define CS35L5X_ASP1_FMT_I2S    0x2
#define CS35L5X_ASP1_FMT_TDM15  0x4
#define CS35L5X_BCLK_FSYNC_MASK GENMASK(6, 0)
#define CS35L5X_ASP1_BCLK_INV   BIT(6)
#define CS35L5X_ASP1_FSYNC_INV  BIT(2)

#define CS35L5X_ASP1_FRAME_CONTROL1 0x4810
#define CS35L5X_ASP1_TX1            1
#define CS35L5X_ASP1_TX2            2
#define CS35L5X_ASP1_TX3            3
#define CS35L5X_ASP1_TX4            4
#define CS35L5X_ASP1_TX1_SLOT       GENMASK(5, 0)
#define CS35L5X_ASP1_TX2_SLOT       GENMASK(13, 8)
#define CS35L5X_ASP1_TX2_SHIFT      8
#define CS35L5X_ASP1_TX3_SLOT       GENMASK(21, 16)
#define CS35L5X_ASP1_TX3_SHIFT      16
#define CS35L5X_ASP1_TX4_SLOT       GENMASK(29, 24)
#define CS35L5X_ASP1_TX4_SHIFT      24

#define CS35L5X_ASP1_FRAME_CONTROL5 0x4820
#define CS35L5X_ASP1_RX1_SLOT       GENMASK(5, 0)
#define CS35L5X_ASP1_RX2_SLOT       GENMASK(13, 8)
#define CS35L5X_ASP1_RX2_SHIFT      8

#define CS35L5X_ASP1_DATA_CONTROL1 0x4830
#define CS35L5X_ASP1_DATA_CONTROL5 0x4840
#define CS35L5X_ASP1_WL_MIN        0xc
#define CS35L5X_ASP1_WL_MAX        0x18

#define CS35L5X_MAIN_RENDER_USER_MUTE   0x3400024
#define CS35L5X_MAIN_RENDER_USER_VOLUME 0x340002C

/* DSP State */
#define CS35L5X_DSP_STATE_RUNNING 2

/* Audio Commands */
#define CS35L5X_DSP_MBOX_CMD_PLAY  0x0B000001
#define CS35L5X_DSP_MBOX_CMD_PAUSE 0x0B000002

/* Timing characteristics */
#define CS35L5X_T_RLPW_US        K_USEC(1000)
#define CS35L5X_T_IRS_US         K_USEC(2200)
#define CS35L5X_ROM_BOOT_POLL_US K_USEC(1000)
#define CS35L5X_ROM_BOOT_RETRIES 250
#define CS35L5X_ROM_BOOT_TIMEOUT K_MSEC(250)

enum cs35l5x_supply {
	CS35L5X_SUPPLY_VDD_P,
	CS35L5X_SUPPLY_VDD_IO,
	CS35L5X_SUPPLY_VDD_A,
	CS35L5X_SUPPLY_VDD_B,
	CS35L5X_SUPPLY_VDD_AMP,
	CS35L5X_NUM_SUPPLIES,
};

static const char *const cs35l5x_supply_names[CS35L5X_NUM_SUPPLIES] = {
	[CS35L5X_SUPPLY_VDD_P] = "VDD_P",     [CS35L5X_SUPPLY_VDD_IO] = "VDD_IO",
	[CS35L5X_SUPPLY_VDD_A] = "VDD_A",     [CS35L5X_SUPPLY_VDD_B] = "VDD_B",
	[CS35L5X_SUPPLY_VDD_AMP] = "VDD_AMP",
};

struct cs35l5x_config {
	LOG_INSTANCE_PTR_DECLARE(log);
	/* Log instance declaration requires blank line. */
	struct gpio_dt_spec reset_gpio;
	const struct device *supplies[CS35L5X_NUM_SUPPLIES];
	const struct cs35lxx_io_bus io_bus;
	uint32_t device_id;
};

static const uint32_t cs35l5x_bclk_freq_hz[] = {
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

static int cs35l5x_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	const struct cs35l5x_config *config = dev->config;
	int ret;

	switch ((int)channel) {
	case CS35L5X_CHANNEL_ASP1_TX1:
		ret = cs35lxx_update(&config->io_bus, CS35L5X_ASP1_FRAME_CONTROL1,
				     CS35L5X_ASP1_TX1_SLOT, input);
		if (ret < 0) {
			return ret;
		}

		return cs35lxx_update(&config->io_bus, CS35L5X_ASP1_ENABLES1, CS35L5X_ASP1_TX1_EN,
				      CS35L5X_ASP1_TX1_EN);

	case CS35L5X_CHANNEL_ASP1_TX2:
		ret = cs35lxx_update(&config->io_bus, CS35L5X_ASP1_FRAME_CONTROL1,
				     CS35L5X_ASP1_TX2_SLOT, input << CS35L5X_ASP1_TX2_SHIFT);
		if (ret < 0) {
			return ret;
		}

		return cs35lxx_update(&config->io_bus, CS35L5X_ASP1_ENABLES1, CS35L5X_ASP1_TX2_EN,
				      CS35L5X_ASP1_TX2_EN);

	case CS35L5X_CHANNEL_ASP1_TX3:
		ret = cs35lxx_update(&config->io_bus, CS35L5X_ASP1_FRAME_CONTROL1,
				     CS35L5X_ASP1_TX3_SLOT, input << CS35L5X_ASP1_TX3_SHIFT);
		if (ret < 0) {
			return ret;
		}

		return cs35lxx_update(&config->io_bus, CS35L5X_ASP1_ENABLES1, CS35L5X_ASP1_TX3_EN,
				      CS35L5X_ASP1_TX3_EN);

	case CS35L5X_CHANNEL_ASP1_TX4:
		ret = cs35lxx_update(&config->io_bus, CS35L5X_ASP1_FRAME_CONTROL1,
				     CS35L5X_ASP1_TX4_SLOT, input << CS35L5X_ASP1_TX4_SHIFT);
		if (ret < 0) {
			return ret;
		}

		return cs35lxx_update(&config->io_bus, CS35L5X_ASP1_ENABLES1, CS35L5X_ASP1_TX4_EN,
				      CS35L5X_ASP1_TX4_EN);

	default:
		return -EINVAL;
	}
}

static int cs35l5x_route_output(const struct device *dev, audio_channel_t channel, uint32_t output)
{
	const struct cs35l5x_config *config = dev->config;
	int ret;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		ret = cs35lxx_update(&config->io_bus, CS35L5X_ASP1_FRAME_CONTROL5,
				     CS35L5X_ASP1_RX1_SLOT, output);
		if (ret < 0) {
			return ret;
		}

		return cs35lxx_update(&config->io_bus, CS35L5X_ASP1_ENABLES1, CS35L5X_ASP1_RX1_EN,
				      CS35L5X_ASP1_RX1_EN);

	case AUDIO_CHANNEL_FRONT_RIGHT:
		ret = cs35lxx_update(&config->io_bus, CS35L5X_ASP1_FRAME_CONTROL5,
				     CS35L5X_ASP1_RX2_SLOT, output << CS35L5X_ASP1_RX2_SHIFT);
		if (ret < 0) {
			return ret;
		}

		return cs35lxx_update(&config->io_bus, CS35L5X_ASP1_ENABLES1, CS35L5X_ASP1_RX2_EN,
				      CS35L5X_ASP1_RX2_EN);

	default:
		return -EINVAL;
	}
}

static int cs35l5x_apply_properties(const struct device *dev)
{
	return 0;
}

static int cs35l5x_output_volume(const struct device *dev, audio_channel_t channel,
				 audio_property_value_t audio_val)
{
	const struct cs35l5x_config *config = dev->config;

	if (channel != AUDIO_CHANNEL_ALL) {
		return -EINVAL;
	}

	return cs35lxx_write(&config->io_bus, CS35L5X_MAIN_RENDER_USER_VOLUME,
			     (uint32_t)audio_val.vol);
}

static int cs35l5x_output_mute(const struct device *dev, audio_channel_t channel,
			       audio_property_value_t audio_val)
{
	const struct cs35l5x_config *config = dev->config;

	if (channel != AUDIO_CHANNEL_ALL) {
		return -EINVAL;
	}

	return cs35lxx_write(&config->io_bus, CS35L5X_MAIN_RENDER_USER_MUTE,
			     (uint32_t)audio_val.mute);
}

static int cs35l5x_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return cs35l5x_output_mute(dev, channel, val);
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return cs35l5x_output_volume(dev, channel, val);
	default:
		return -ENOTSUP;
	}
}

static void cs35l5x_stop_output(const struct device *dev)
{
	const struct cs35l5x_config *config = dev->config;

	(void)cs35lxx_write(&config->io_bus, CS35L5X_DSP_VIRTUAL1_MBOX_1,
			    CS35L5X_DSP_MBOX_CMD_PAUSE);
}

static void cs35l5x_start_output(const struct device *dev)
{
	const struct cs35l5x_config *config = dev->config;

	(void)cs35lxx_write(&config->io_bus, CS35L5X_DSP_VIRTUAL1_MBOX_1,
			    CS35L5X_DSP_MBOX_CMD_PLAY);
}

static int cs35l5x_asp1_set_clks(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct cs35l5x_config *config = dev->config;
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

	for (int i = CS35L5X_ASP_BCLK_FREQ_OFFSET; i < ARRAY_SIZE(cs35l5x_bclk_freq_hz); i++) {
		if (cs35l5x_bclk_freq_hz[i] == bclk_freq_hz) {
			asp1_bclk_freq = i;
			break;
		}
	}

	if (asp1_bclk_freq == 0) {
		return -EINVAL;
	}

	ret = cs35lxx_write(&config->io_bus, CS35L5X_ASP1_CONTROL1, asp1_bclk_freq);
	if (ret < 0) {
		return ret;
	}

	if (FIELD_GET(I2S_FMT_BIT_CLK_INV, i2s.format) > 0) {
		clk_opt |= CS35L5X_ASP1_BCLK_INV;
	}

	if (FIELD_GET(I2S_FMT_FRAME_CLK_INV, i2s.format) > 0) {
		clk_opt |= CS35L5X_ASP1_FSYNC_INV;
	}

	return cs35lxx_update(&config->io_bus, CS35L5X_ASP1_CONTROL2, CS35L5X_BCLK_FSYNC_MASK,
			      clk_opt);
}

static int cs35l5x_asp1_set_word(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct cs35l5x_config *config = dev->config;
	struct i2s_config i2s = cfg->dai_cfg.i2s;
	uint8_t asp1_width;
	uint32_t val = 0;
	int ret;

	if (!IN_RANGE(i2s.word_size, CS35L5X_ASP1_WL_MIN, CS35L5X_ASP1_WL_MAX)) {
		return -EINVAL;
	}

	if (i2s.word_size == AUDIO_PCM_WIDTH_16_BITS) {
		asp1_width = AUDIO_PCM_WIDTH_16_BITS;
	} else {
		asp1_width = AUDIO_PCM_WIDTH_32_BITS;
	}

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		ret = cs35lxx_write(&config->io_bus, CS35L5X_ASP1_DATA_CONTROL1, i2s.word_size);
		if (ret < 0) {
			return ret;
		}

		val |= FIELD_PREP(CS35L5X_ASP1_TX_WIDTH, asp1_width);
		__fallthrough;
	case AUDIO_ROUTE_PLAYBACK:
		ret = cs35lxx_write(&config->io_bus, CS35L5X_ASP1_DATA_CONTROL5, i2s.word_size);
		if (ret < 0) {
			return ret;
		}

		val |= FIELD_PREP(CS35L5X_ASP1_RX_WIDTH, asp1_width);
		break;
	default:
		return -EINVAL;
	}

	switch (FIELD_GET(I2S_FMT_DATA_FORMAT_MASK, i2s.format)) {
	case I2S_FMT_DATA_FORMAT_I2S:
		val |= FIELD_PREP(CS35L5X_ASP1_FMT_MASK, CS35L5X_ASP1_FMT_I2S);
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
		val |= FIELD_PREP(CS35L5X_ASP1_FMT_MASK, CS35L5X_ASP1_FMT_TDM15);
		break;
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		val |= FIELD_PREP(CS35L5X_ASP1_FMT_MASK, CS35L5X_ASP1_FMT_DSPA);
		break;
	default:
		return -ENOTSUP;
	}

	return cs35lxx_update(&config->io_bus, CS35L5X_ASP1_CONTROL2,
			      (CS35L5X_ASP1_FMT_MASK | CS35L5X_ASP1_WIDTH), val);
}

static int cs35l5x_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	__maybe_unused const struct cs35l5x_config *config = dev->config;
	int ret;

	ret = cs35l5x_asp1_set_clks(dev, cfg);
	if (ret < 0) {
		LOG_INST_ERR(config->log, "Failed to set clocks");
		return ret;
	}

	ret = cs35l5x_asp1_set_word(dev, cfg);
	if (ret < 0) {
		LOG_INST_ERR(config->log, "Failed to set word");
		return ret;
	}

	return 0;
}

static int cs35l5x_wait_for_rom_boot(const struct device *dev)
{
	const k_timepoint_t end = sys_timepoint_calc(CS35L5X_ROM_BOOT_TIMEOUT);
	const struct cs35l5x_config *config = dev->config;
	uint32_t val = 0;
	int ret;

	do {
		ret = cs35lxx_read(&config->io_bus, CS35L5X_HALO_STATE, &val);
		if (ret < 0) {
			return ret;
		}

		if (val == CS35L5X_DSP_STATE_RUNNING) {
			return 0;
		}

		(void)k_sleep(CS35L5X_ROM_BOOT_POLL_US);
	} while (!sys_timepoint_expired(end));

	return -EPERM;
}

static int cs35l5x_reset(const struct device *dev)
{
	const struct cs35l5x_config *config = dev->config;
	int ret;

	if ((config->reset_gpio.port != NULL) && IS_ENABLED(CONFIG_AUDIO_CODEC_CS35L5X_RESET)) {
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		(void)k_sleep(CS35L5X_T_RLPW_US);

		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}

		(void)k_sleep(CS35L5X_T_IRS_US);
	} else {
		/*
		 * Note that the DSP firmware memory (RAM) contents are retained through software
		 * reset conditions.
		 */
		ret = cs35lxx_write(&config->io_bus, CS35L5X_SW_RESET_SFT_RESET_REG, 0x5A000000);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cs35l5x_check_ids(const struct device *dev)
{
	const struct cs35l5x_config *config = dev->config;
	uint32_t val;
	int ret;

	ret = cs35lxx_read(&config->io_bus, CS35L5X_SW_RESET_DEVID_REG, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != config->device_id) {
		return -EINVAL;
	}

	ret = cs35lxx_read(&config->io_bus, CS35L5X_SW_RESET_REVID_REG, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != 0xB2) {
		return -EINVAL;
	}

	LOG_INST_INF(config->log, "Cirrus Logic CS35L%02X Revision %02X",
		     (uint8_t)config->device_id, (uint8_t)val);

	return 0;
}

static int cs35l5x_enable_regulator(const struct device *dev, const int i)
{
	const struct cs35l5x_config *config = dev->config;
	const struct device *regulator = config->supplies[i];

	if (regulator == NULL) {
		LOG_INST_DBG(config->log, "%s regulator not provided", cs35l5x_supply_names[i]);
		return 0;
	}

	if (!device_is_ready(regulator)) {
		LOG_INST_ERR(config->log, "%s regulator device %s is not ready",
			     cs35l5x_supply_names[i], regulator->name);
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_AUDIO_CODEC_CS35L5X_REGULATOR)) {
		int ret = regulator_enable(regulator);

		if (ret < 0) {
			LOG_INST_ERR(config->log, "Failed to enable %s regulator: %d",
				     cs35l5x_supply_names[i], ret);
			return ret;
		}
	}

	return 0;
}

static void cs35l5x_disable_regulator(const struct device *dev, const int i)
{
	const struct cs35l5x_config *config = dev->config;
	const struct device *regulator = config->supplies[i];

	if ((regulator != NULL) && IS_ENABLED(CONFIG_AUDIO_CODEC_CS35L5X_REGULATOR)) {
		(void)regulator_disable(regulator);
	}
}

static void cs35l5x_disable_regulators(const struct device *dev)
{
	for (int i = 0; i < ARRAY_SIZE(cs35l5x_supply_names); i++) {
		cs35l5x_disable_regulator(dev, i);
	}
}

static int cs35l5x_init_regulators(const struct device *dev)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(cs35l5x_supply_names); i++) {
		ret = cs35l5x_enable_regulator(dev, i);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int cs35l5x_init(const struct device *dev)
{
	const struct cs35l5x_config *config = dev->config;
	int ret;

	if (!cs35lxx_is_bus_ready(&config->io_bus)) {
		LOG_INST_ERR(config->log, "Control port %s is not ready",
			     cs35lxx_get_control_port(&config->io_bus)->name);
		return -ENODEV;
	}

	ret = cs35l5x_init_regulators(dev);
	if (ret < 0) {
		LOG_INST_ERR(config->log, "Failed to enable regulators: %d", ret);
		return ret;
	}

	ret = cs35l5x_reset(dev);
	if (ret < 0) {
		LOG_INST_ERR(config->log, "Fail to reset: %d", ret);
		goto error_regulators;
	}

	ret = cs35l5x_wait_for_rom_boot(dev);
	if (ret < 0) {
		LOG_INST_ERR(config->log, "Failed to boot from ROM: %d", ret);
		goto error_regulators;
	}

	ret = cs35l5x_check_ids(dev);
	if (ret < 0) {
		LOG_INST_ERR(config->log, "Failed to check IDs: %d", ret);
		goto error_regulators;
	}

	ret = cs35lxx_update(&config->io_bus, CS35L5X_BLOCK_ENABLES2, CS35L5X_ASP_EN,
			     CS35L5X_ASP_EN);
	if (ret < 0) {
		goto error_regulators;
	}

	return 0;

error_regulators:
	cs35l5x_disable_regulators(dev);

	return ret;
}

static DEVICE_API(audio_codec, api) = {
	.configure = cs35l5x_configure,
	.start_output = cs35l5x_start_output,
	.stop_output = cs35l5x_stop_output,
	.set_property = cs35l5x_set_property,
	.apply_properties = cs35l5x_apply_properties,
	.route_input = cs35l5x_route_input,
	.route_output = cs35l5x_route_output,
};

#define CS35L5X_DEVICE_INIT(inst, name)                                                            \
	DEVICE_DT_INST_DEFINE(inst, cs35l5x_init, NULL, NULL, &name##_config_##inst, POST_KERNEL,  \
			      CONFIG_AUDIO_CODEC_CS35L5X_INIT_PRIORITY, &api);

#define CS35L5X_BUS(inst)                                                                          \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),                                                   \
		(.io_bus.bus.i2c = I2C_DT_SPEC_INST_GET(inst), .io_bus.io = &cs35lxx_io_i2c),    \
		(.io_bus.bus.spi = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER),               \
		 .io_bus.io = &cs35lxx_io_spi))

#define CS35L5X_SUPPLIES(inst)                                                                     \
	[CS35L5X_SUPPLY_VDD_P] = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, vdd_p_supply)),       \
	[CS35L5X_SUPPLY_VDD_IO] = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, vdd_io_supply)),     \
	[CS35L5X_SUPPLY_VDD_A] = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, vdd_a_supply)),       \
	[CS35L5X_SUPPLY_VDD_B] = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, vdd_b_supply)),       \
	[CS35L5X_SUPPLY_VDD_AMP] = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, vdd_amp_supply))

#define cs35l5x_config(inst, id)                                                                   \
	{                                                                                          \
		LOG_INSTANCE_PTR_INIT(log, DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(inst)), inst)       \
			CS35L5X_BUS(inst),                                                         \
		.supplies = {CS35L5X_SUPPLIES(inst)},                                              \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.device_id = id,                                                                   \
	}

#define AUDIO_CODEC_CS35L5X_DEFINE(inst, name, id)                                                 \
	LOG_INSTANCE_REGISTER(DT_NODE_FULL_NAME_TOKEN(DT_DRV_INST(inst)), inst,                    \
			      CONFIG_AUDIO_CODEC_LOG_LEVEL);                                       \
	static const struct cs35l5x_config name##_config_##inst = cs35l5x_config(inst, id);        \
	CS35L5X_DEVICE_INIT(inst, name)

#define DT_DRV_COMPAT cirrus_cs35l56
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define CS35L56_NAME      cs35l56
#define CS35L56_DEVICE_ID 0x35A56
DT_INST_FOREACH_STATUS_OKAY_VARGS(AUDIO_CODEC_CS35L5X_DEFINE, CS35L56_NAME, CS35L56_DEVICE_ID)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT cirrus_cs35l57
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define CS35L57_NAME      cs35l57
#define CS35L57_DEVICE_ID 0x35A57
DT_INST_FOREACH_STATUS_OKAY_VARGS(AUDIO_CODEC_CS35L5X_DEFINE, CS35L57_NAME, CS35L57_DEVICE_ID)
#endif
#undef DT_DRV_COMPAT
