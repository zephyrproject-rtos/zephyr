/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlv320dac

#include <errno.h>
#include <math.h>

#include <zephyr/sys/util.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include "tlv320dac310x.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlv320dac310x);

#define CODEC_OUTPUT_VOLUME_MAX		0
#define CODEC_OUTPUT_VOLUME_MIN		(-78 * 2)

/*
 * TLV320DAC3100: compact clock table, PLL source = BCLK, D = 0,
 * slot width = word width (BCLK = 2 * bits * fs).
 *
 * Key   (32 bit): fs << 8 | bits               (e.g. 48 kHz, 32 bit -> 0x00BB8020)
 * Value (32 bit): [30:21] DOSR   [20:17] NDAC   [16:13] MDAC
 *                 [12:7]  J      [6:3]   R      [2:0]   P        bit 31 unused
 *
 * NDAC and MDAC are limited to 1..15, which is sufficient for all entries. Combinations that are
 * not possible (BCLK/P < 512 kHz) are absent from the table; the lookup returns false.
 */

#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_internal_pll_with_bclk)

#define DAC3100_KEY(fs, bits) (((uint32_t)(fs) << 8) | (uint32_t)(bits))

#define DAC3100_VAL(p, r, j, ndac, mdac, dosr)                                                     \
	((((uint32_t)(p) & 7u) << 0) | (((uint32_t)(r) & 15u) << 3) |                              \
	 (((uint32_t)(j) & 63u) << 7) | (((uint32_t)(mdac) & 15u) << 13) |                         \
	 (((uint32_t)(ndac) & 15u) << 17) | (((uint32_t)(dosr) & 1023u) << 21))

/* Field extraction. */
#define DAC3100_P(v)    (((uint32_t)(v) >> 0) & 7u)
#define DAC3100_R(v)    (((uint32_t)(v) >> 3) & 15u)
#define DAC3100_J(v)    (((uint32_t)(v) >> 7) & 63u)
#define DAC3100_MDAC(v) (((uint32_t)(v) >> 13) & 15u)
#define DAC3100_NDAC(v) (((uint32_t)(v) >> 17) & 15u)
#define DAC3100_DOSR(v) (((uint32_t)(v) >> 21) & 1023u)

struct dac3100_clk_entry {
	uint32_t key;
	uint32_t val;
};

/* Sorted by ascending key. */
static const struct dac3100_clk_entry dac3100_clk_table[] = {
	{DAC3100_KEY(8000, 32), DAC3100_VAL(1, 3, 56, 14, 1, 768)},
	{DAC3100_KEY(11025, 24), DAC3100_VAL(1, 4, 40, 15, 1, 512)},
	{DAC3100_KEY(11025, 32), DAC3100_VAL(1, 2, 60, 15, 1, 512)},
	{DAC3100_KEY(12000, 24), DAC3100_VAL(1, 4, 40, 15, 1, 512)},
	{DAC3100_KEY(12000, 32), DAC3100_VAL(1, 2, 56, 14, 1, 512)},
	{DAC3100_KEY(16000, 16), DAC3100_VAL(1, 3, 56, 14, 1, 384)},
	{DAC3100_KEY(16000, 20), DAC3100_VAL(1, 3, 48, 15, 1, 384)},
	{DAC3100_KEY(16000, 24), DAC3100_VAL(1, 2, 56, 14, 1, 384)},
	{DAC3100_KEY(16000, 32), DAC3100_VAL(1, 2, 42, 14, 1, 384)},
	{DAC3100_KEY(22050, 16), DAC3100_VAL(1, 2, 60, 5, 3, 256)},
	{DAC3100_KEY(22050, 20), DAC3100_VAL(1, 2, 48, 5, 3, 256)},
	{DAC3100_KEY(22050, 24), DAC3100_VAL(1, 2, 40, 5, 3, 256)},
	{DAC3100_KEY(22050, 32), DAC3100_VAL(1, 1, 60, 5, 3, 256)},
	{DAC3100_KEY(24000, 16), DAC3100_VAL(1, 2, 56, 7, 2, 256)},
	{DAC3100_KEY(24000, 20), DAC3100_VAL(1, 2, 48, 5, 3, 256)},
	{DAC3100_KEY(24000, 24), DAC3100_VAL(1, 2, 40, 5, 3, 256)},
	{DAC3100_KEY(24000, 32), DAC3100_VAL(1, 1, 56, 7, 2, 256)},
	{DAC3100_KEY(32000, 16), DAC3100_VAL(1, 2, 42, 7, 2, 192)},
	{DAC3100_KEY(32000, 20), DAC3100_VAL(1, 2, 36, 5, 3, 192)},
	{DAC3100_KEY(32000, 24), DAC3100_VAL(1, 1, 56, 7, 2, 192)},
	{DAC3100_KEY(32000, 32), DAC3100_VAL(1, 1, 42, 7, 2, 192)},
	{DAC3100_KEY(44100, 16), DAC3100_VAL(1, 1, 60, 5, 3, 128)},
	{DAC3100_KEY(44100, 20), DAC3100_VAL(1, 1, 48, 5, 3, 128)},
	{DAC3100_KEY(44100, 24), DAC3100_VAL(1, 1, 40, 5, 3, 128)},
	{DAC3100_KEY(44100, 32), DAC3100_VAL(1, 1, 30, 5, 3, 128)},
	{DAC3100_KEY(48000, 16), DAC3100_VAL(1, 1, 56, 2, 7, 128)},
	{DAC3100_KEY(48000, 20), DAC3100_VAL(1, 1, 48, 5, 3, 128)},
	{DAC3100_KEY(48000, 24), DAC3100_VAL(1, 1, 40, 5, 3, 128)},
	{DAC3100_KEY(48000, 32), DAC3100_VAL(1, 1, 28, 2, 7, 128)},
	{DAC3100_KEY(88200, 16), DAC3100_VAL(1, 1, 30, 3, 5, 64)},
	{DAC3100_KEY(88200, 20), DAC3100_VAL(1, 1, 24, 3, 5, 64)},
	{DAC3100_KEY(88200, 24), DAC3100_VAL(1, 1, 20, 3, 5, 64)},
	{DAC3100_KEY(88200, 32), DAC3100_VAL(1, 1, 15, 3, 5, 64)},
	{DAC3100_KEY(96000, 16), DAC3100_VAL(1, 1, 28, 2, 7, 64)},
	{DAC3100_KEY(96000, 20), DAC3100_VAL(1, 1, 24, 3, 5, 64)},
	{DAC3100_KEY(96000, 24), DAC3100_VAL(1, 1, 20, 3, 5, 64)},
	{DAC3100_KEY(96000, 32), DAC3100_VAL(1, 1, 14, 2, 7, 64)},
	{DAC3100_KEY(176400, 16), DAC3100_VAL(1, 1, 16, 2, 8, 32)},
	{DAC3100_KEY(176400, 20), DAC3100_VAL(5, 2, 32, 2, 8, 32)},
	{DAC3100_KEY(176400, 24), DAC3100_VAL(1, 1, 12, 3, 6, 32)},
	{DAC3100_KEY(176400, 32), DAC3100_VAL(1, 1, 8, 2, 8, 32)},
	{DAC3100_KEY(192000, 16), DAC3100_VAL(1, 1, 14, 2, 7, 32)},
	{DAC3100_KEY(192000, 20), DAC3100_VAL(5, 1, 56, 2, 7, 32)},
	{DAC3100_KEY(192000, 24), DAC3100_VAL(3, 1, 28, 2, 7, 32)},
	{DAC3100_KEY(192000, 32), DAC3100_VAL(1, 1, 7, 2, 7, 32)},
};

#endif /* DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_internal_pll_with_bclk) */

static void codec_soft_reset(const struct device *dev);
static int codec_configure_dai(const struct device *dev, audio_dai_cfg_t *cfg);
static int codec_configure_clocks(const struct device *dev,
				  struct audio_codec_cfg *cfg);
static void codec_configure_filters(const struct device *dev,
				   audio_dai_cfg_t *cfg);
static enum osr_multiple codec_get_osr_multiple(audio_dai_cfg_t *cfg);
static void codec_configure_output(const struct device *dev);
static int codec_set_output_volume(const struct device *dev, int vol, audio_channel_t channel);

static void codec_write_reg(const struct device *dev, struct reg_addr reg, uint8_t val);
static void codec_read_reg(const struct device *dev, struct reg_addr reg, uint8_t *val);

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
static void codec_read_all_regs(const struct device *dev);
#define CODEC_DUMP_REGS(dev)	codec_read_all_regs((dev))
#else
#define CODEC_DUMP_REGS(dev)
#endif

static int codec_initialize(const struct device *dev)
{
	const struct tlv320dac310x_config *const dev_cfg = dev->config;

	if (!device_is_ready(dev_cfg->bus.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&dev_cfg->reset_gpio)) {
		LOG_ERR("No device reset gpio, have to use soft reset");
	}

	return 0;
}

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct tlv320dac310x_config *const dev_cfg = dev->config;
	int ret;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("dai_type must be AUDIO_DAI_TYPE_I2S");
		return -EINVAL;
	}

	if (gpio_is_ready_dt(&dev_cfg->reset_gpio)) {
		ret = gpio_pin_configure_dt(&dev_cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}

		k_usleep(1);

		ret = gpio_pin_configure_dt(&dev_cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO (%d)", ret);
			return ret;
		}
	} else {
		codec_soft_reset(dev);
	}

	k_msleep(10);

	ret = codec_configure_clocks(dev, cfg);
	if (ret == 0) {
		ret = codec_configure_dai(dev, &cfg->dai_cfg);
	}
	if (ret == 0) {
		codec_configure_filters(dev, &cfg->dai_cfg);
	}
	codec_configure_output(dev);

	return ret;
}

static void codec_start_output(const struct device *dev)
{
	/* powerup DAC channels */
	codec_write_reg(dev, DATA_PATH_SETUP_ADDR, DAC_LR_POWERUP_DEFAULT);

	/* unmute DAC channels */
	codec_write_reg(dev, VOL_CTRL_ADDR, VOL_CTRL_UNMUTE_DEFAULT);

	CODEC_DUMP_REGS(dev);
}

static void codec_stop_output(const struct device *dev)
{
	/* mute DAC channels */
	codec_write_reg(dev, VOL_CTRL_ADDR, VOL_CTRL_MUTE_DEFAULT);

	/* powerdown DAC channels */
	codec_write_reg(dev, DATA_PATH_SETUP_ADDR, DAC_LR_POWERDN_DEFAULT);
}

static void codec_mute_output(const struct device *dev, audio_channel_t channel)
{
	uint8_t val;

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_HEADPHONE_LEFT)) {
		codec_write_reg(dev, HPL_DRV_GAIN_CTRL_ADDR, HPX_DRV_MUTE);
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_HEADPHONE_RIGHT)) {
		codec_write_reg(dev, HPR_DRV_GAIN_CTRL_ADDR, HPX_DRV_MUTE);
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_FRONT_LEFT) ||
	    (channel == AUDIO_CHANNEL_REAR_LEFT) ||
	    (channel == AUDIO_CHANNEL_SIDE_LEFT)) {
		codec_read_reg(dev, SPKL_DRV_ADDR, &val);
		codec_write_reg(dev, SPKL_DRV_ADDR, val & ~SPEAKER_DRV_UNMUTE);
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_FRONT_RIGHT) ||
	    (channel == AUDIO_CHANNEL_REAR_RIGHT) ||
	    (channel == AUDIO_CHANNEL_SIDE_RIGHT)) {
		codec_read_reg(dev, SPKR_DRV_ADDR, &val);
		codec_write_reg(dev, SPKR_DRV_ADDR, val & ~SPEAKER_DRV_UNMUTE);
	}
}

static void codec_unmute_output(const struct device *dev, audio_channel_t channel)
{
	uint8_t val;

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_HEADPHONE_LEFT)) {
		codec_write_reg(dev, HPL_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE);
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_HEADPHONE_RIGHT)) {
		codec_write_reg(dev, HPR_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE);
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_FRONT_LEFT) ||
	    (channel == AUDIO_CHANNEL_REAR_LEFT) ||
	    (channel == AUDIO_CHANNEL_SIDE_LEFT)) {
		codec_read_reg(dev, SPKL_DRV_ADDR, &val);
		codec_write_reg(dev, SPKL_DRV_ADDR, val | SPEAKER_DRV_UNMUTE);
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_FRONT_RIGHT) ||
	    (channel == AUDIO_CHANNEL_REAR_RIGHT) ||
	    (channel == AUDIO_CHANNEL_SIDE_RIGHT)) {
		codec_read_reg(dev, SPKR_DRV_ADDR, &val);
		codec_write_reg(dev, SPKR_DRV_ADDR, val | SPEAKER_DRV_UNMUTE);
	}
}

static int codec_set_property(const struct device *dev,
			      audio_property_t property,
			      audio_channel_t channel,
			      audio_property_value_t val)
{
	if ((channel == AUDIO_CHANNEL_LFE) ||
	    (channel == AUDIO_CHANNEL_FRONT_CENTER) ||
	    (channel == AUDIO_CHANNEL_REAR_CENTER)) {
		LOG_ERR("channel %u not supported", channel);
		return -ENOTSUP;
	}

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return codec_set_output_volume(dev, val.vol, channel);

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (val.mute) {
			codec_mute_output(dev, channel);
		} else {
			codec_unmute_output(dev, channel);
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

static void codec_write_reg(const struct device *dev, struct reg_addr reg,
			    uint8_t val)
{
	struct tlv320dac310x_data *const dev_data = dev->data;
	const struct tlv320dac310x_config *const dev_cfg = dev->config;

	/* set page if different */
	if (dev_data->reg_addr_cache.page != reg.page) {
		i2c_reg_write_byte_dt(&dev_cfg->bus, 0, reg.page);
		dev_data->reg_addr_cache.page = reg.page;
	}

	i2c_reg_write_byte_dt(&dev_cfg->bus, reg.reg_addr, val);
	LOG_DBG("WR PG:%u REG:%02u VAL:0x%02x",
			reg.page, reg.reg_addr, val);
}

static void codec_read_reg(const struct device *dev, struct reg_addr reg,
			   uint8_t *val)
{
	struct tlv320dac310x_data *const dev_data = dev->data;
	const struct tlv320dac310x_config *const dev_cfg = dev->config;

	/* set page if different */
	if (dev_data->reg_addr_cache.page != reg.page) {
		i2c_reg_write_byte_dt(&dev_cfg->bus, 0, reg.page);
		dev_data->reg_addr_cache.page = reg.page;
	}

	i2c_reg_read_byte_dt(&dev_cfg->bus, reg.reg_addr, val);
	LOG_DBG("RD PG:%u REG:%02u VAL:0x%02x",
			reg.page, reg.reg_addr, *val);
}

static void codec_soft_reset(const struct device *dev)
{
	/* soft reset the DAC */
	codec_write_reg(dev, SOFT_RESET_ADDR, SOFT_RESET_ASSERT);
}

static int codec_configure_dai(const struct device *dev, audio_dai_cfg_t *cfg)
{
	uint8_t val;
#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_internal_pll_with_bclk)
	const struct tlv320dac310x_config *const dev_cfg = dev->config;

	if (dev_cfg->use_internal_pll_with_bclk) {
		if (cfg->i2s.options & I2S_OPT_BIT_CLK_CONTROLLER) {
			LOG_ERR("Cannot set BCLK as output when PLL is clocked by BCLK");
			return -EINVAL;
		}
	}
#endif

	/* configure I2S interface */
	val = IF_CTRL_IFTYPE(IF_CTRL_IFTYPE_I2S);
	if (cfg->i2s.options & I2S_OPT_BIT_CLK_CONTROLLER) {
		val |= IF_CTRL_BCLK_OUT;
	}

	if (cfg->i2s.options & I2S_OPT_FRAME_CLK_CONTROLLER) {
		val |= IF_CTRL_WCLK_OUT;
	}

	switch (cfg->i2s.word_size) {
	case AUDIO_PCM_WIDTH_16_BITS:
		val |= IF_CTRL_WLEN(IF_CTRL_WLEN_16);
		break;
	case AUDIO_PCM_WIDTH_20_BITS:
		val |= IF_CTRL_WLEN(IF_CTRL_WLEN_20);
		break;
	case AUDIO_PCM_WIDTH_24_BITS:
		val |= IF_CTRL_WLEN(IF_CTRL_WLEN_24);
		break;
	case AUDIO_PCM_WIDTH_32_BITS:
		val |= IF_CTRL_WLEN(IF_CTRL_WLEN_32);
		break;
	default:
		LOG_ERR("Unsupported PCM sample bit width %u",
				cfg->i2s.word_size);
		return -EINVAL;
	}

	codec_write_reg(dev, IF_CTRL1_ADDR, val);
	return 0;
}

#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_internal_pll_with_bclk)

static bool dac3100_clk_lookup(uint32_t fs, uint32_t bits, uint32_t *val)
{
	const uint32_t key = DAC3100_KEY(fs, bits);

	for (size_t i = 0; i < ARRAY_SIZE(dac3100_clk_table); i++) {
		if (dac3100_clk_table[i].key == key) {
			*val = dac3100_clk_table[i].val;
			return true;
		}
	}
	return false;
}

struct dac310x_pll_cfg {
	uint8_t p;
	uint8_t r;
	uint8_t j;
};

static bool codec_configure_pll(const struct device *dev, const struct dac310x_pll_cfg *pll)
{
	/* Validate all input ranges */
	if (pll->p < 1 || pll->p > 8) {
		return false;
	}
	if (pll->r < 1 || pll->r > 16) {
		return false;
	}
	if (pll->j < 1 || pll->j > 63) {
		return false;
	}
	codec_write_reg(dev, PLL_P_R_ADDR, PLL_P_DIV(pll->p) | PLL_R_MUL(pll->r));
	codec_write_reg(dev, PLL_J_ADDR, PLL_J(pll->j));

	return true;
}

#endif

static int codec_configure_clocks(const struct device *dev, struct audio_codec_cfg *cfg)
{
#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_internal_pll_with_bclk)
	const struct tlv320dac310x_config *const dev_cfg = dev->config;
#endif
	int dac_clk, mod_clk;
	struct i2s_config *i2s;
	int osr, osr_min, osr_max;
	enum osr_multiple osr_multiple;
	int mdac, ndac, bclk_div, mclk_div;

	i2s = &cfg->dai_cfg.i2s;

#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_internal_pll_with_bclk)
	if (dev_cfg->use_internal_pll_with_bclk) {
		uint8_t pll_p_r = 0;
		uint32_t dac3100_config = 0;

		if (!dac3100_clk_lookup(i2s->frame_clk_freq, i2s->word_size, &dac3100_config)) {
			LOG_ERR("Unable to find suitable pll values");
			return -EINVAL;
		}

		struct dac310x_pll_cfg pll_config = {.p = DAC3100_P(dac3100_config),
						     .r = DAC3100_R(dac3100_config),
						     .j = DAC3100_J(dac3100_config)};

		mdac = DAC3100_MDAC(dac3100_config);
		ndac = DAC3100_NDAC(dac3100_config);
		osr = DAC3100_DOSR(dac3100_config);

		mod_clk = i2s->frame_clk_freq * osr;
		dac_clk = mod_clk * mdac;

		if (i2s->options & I2S_OPT_BIT_CLK_CONTROLLER) {
			LOG_ERR("Internal PLL with BCLK requires bit clock target mode");
			return -EINVAL;
		}

		codec_write_reg(dev, CLOCK_GEN_MUX_ADDR,
				CLOCK_PLL_CLKIN(CLOCK_PLL_CLKIN_BCLK) |
					CLOCK_CODEC_CLKIN(CLOCK_CODEC_CLKIN_PLL_CLK));

		codec_configure_pll(dev, &pll_config);

		/* set NDAC, then MDAC, followed by OSR */
		codec_write_reg(dev, NDAC_DIV_ADDR, (uint8_t)(NDAC_DIV(ndac) | NDAC_POWER_UP_MASK));
		codec_write_reg(dev, MDAC_DIV_ADDR, (uint8_t)(MDAC_DIV(mdac) | MDAC_POWER_UP_MASK));
		codec_write_reg(dev, OSR_MSB_ADDR, (uint8_t)((osr >> 8) & OSR_MSB_MASK));
		codec_write_reg(dev, OSR_LSB_ADDR, (uint8_t)(osr & OSR_LSB_MASK));

		/* Power PLL */
		codec_read_reg(dev, PLL_P_R_ADDR, &pll_p_r);
		pll_p_r |= PLL_POWER_UP;
		codec_write_reg(dev, PLL_P_R_ADDR, pll_p_r);

		return 0;
	}
#endif /* DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_internal_pll_with_bclk) */

	LOG_DBG("MCLK %u Hz PCM Rate: %u Hz", cfg->mclk_freq, i2s->frame_clk_freq);
	if (cfg->mclk_freq <= DAC_PROC_CLK_FREQ_MAX) {
		/* use MCLK frequency as the DAC processing clock */
		ndac = 1;
	} else {
		ndac = cfg->mclk_freq / DAC_PROC_CLK_FREQ_MAX;
	}

	dac_clk = cfg->mclk_freq / ndac;

	/* determine OSR Multiple based on PCM rate */
	osr_multiple = codec_get_osr_multiple(&cfg->dai_cfg);

	/*
	 * calculate MOD clock such that it is an integer multiple of
	 * cfg->i2s.frame_clk_freq and
	 * DAC_MOD_CLK_FREQ_MIN <= MOD clock <= DAC_MOD_CLK_FREQ_MAX
	 */
	osr_min = (DAC_MOD_CLK_FREQ_MIN + i2s->frame_clk_freq - 1) / i2s->frame_clk_freq;
	osr_max = DAC_MOD_CLK_FREQ_MAX / i2s->frame_clk_freq;

	/* round min and max values to the required multiple */
	osr_max = (osr_max / osr_multiple) * osr_multiple;
	osr_min = DIV_ROUND_UP(osr_min, osr_multiple);

	osr = osr_max;
	while (osr >= osr_min) {
		mod_clk = i2s->frame_clk_freq * osr;

		/* calculate mdac */
		mdac = dac_clk / mod_clk;

		/* check if mdac is an integer */
		if ((mdac * mod_clk) == dac_clk) {
			/* found suitable dividers */
			break;
		}
		osr -= osr_multiple;
	}

	/* check if suitable value was found */
	if (osr < osr_min) {
		LOG_ERR("Unable to find suitable mdac and osr values");
		return -EINVAL;
	}

	LOG_DBG("Processing freq: %u Hz Modulator freq: %u Hz", dac_clk, mod_clk);
	LOG_DBG("NDAC: %u MDAC: %u OSR: %u", ndac, mdac, osr);

	if (i2s->options & I2S_OPT_BIT_CLK_CONTROLLER) {
		bclk_div = osr * mdac / (i2s->word_size * 2U); /* stereo */
		if ((bclk_div * i2s->word_size * 2) != (osr * mdac)) {
			LOG_ERR("Unable to generate BCLK %u from MCLK %u",
				i2s->frame_clk_freq * i2s->word_size * 2U, cfg->mclk_freq);
			return -EINVAL;
		}
		LOG_DBG("I2S controller BCLKDIV: %u", bclk_div);
		codec_write_reg(dev, BCLK_DIV_ADDR, BCLK_DIV_POWER_UP | BCLK_DIV(bclk_div));
	}

	/* set NDAC, then MDAC, followed by OSR */
	codec_write_reg(dev, NDAC_DIV_ADDR, (uint8_t)(NDAC_DIV(ndac) | NDAC_POWER_UP_MASK));
	codec_write_reg(dev, MDAC_DIV_ADDR, (uint8_t)(MDAC_DIV(mdac) | MDAC_POWER_UP_MASK));
	codec_write_reg(dev, OSR_MSB_ADDR, (uint8_t)((osr >> 8) & OSR_MSB_MASK));
	codec_write_reg(dev, OSR_LSB_ADDR, (uint8_t)(osr & OSR_LSB_MASK));

	if (i2s->options & I2S_OPT_BIT_CLK_CONTROLLER) {
		codec_write_reg(dev, BCLK_DIV_ADDR, BCLK_DIV(bclk_div) | BCLK_DIV_POWER_UP);
	}

	/* calculate MCLK divider to get ~1MHz */
	mclk_div = DIV_ROUND_UP(cfg->mclk_freq, 1000000);
	/* setup timer clock to be MCLK divided */
	codec_write_reg(dev, TIMER_MCLK_DIV_ADDR,
			TIMER_MCLK_DIV_EN_EXT | TIMER_MCLK_DIV_VAL(mclk_div));
	LOG_DBG("Timer MCLK Divider: %u", mclk_div);

	return 0;
}

static void codec_configure_filters(const struct device *dev,
				   audio_dai_cfg_t *cfg)
{
	enum proc_block proc_blk;

	/* determine decimation filter type */
	if (cfg->i2s.frame_clk_freq >= AUDIO_PCM_RATE_192K) {
		proc_blk = PRB_P18_DECIMATION_C;
		LOG_INF("PCM Rate: %u Filter C PRB P18 selected",
				cfg->i2s.frame_clk_freq);
	} else if (cfg->i2s.frame_clk_freq >= AUDIO_PCM_RATE_96K) {
		proc_blk = PRB_P10_DECIMATION_B;
		LOG_INF("PCM Rate: %u Filter B PRB P10 selected",
				cfg->i2s.frame_clk_freq);
	} else {
		proc_blk = PRB_P25_DECIMATION_A;
		LOG_INF("PCM Rate: %u Filter A PRB P25 selected",
				cfg->i2s.frame_clk_freq);
	}

	codec_write_reg(dev, PROC_BLK_SEL_ADDR, PROC_BLK_SEL(proc_blk));
}

static enum osr_multiple codec_get_osr_multiple(audio_dai_cfg_t *cfg)
{
	enum osr_multiple osr;

	if (cfg->i2s.frame_clk_freq >= AUDIO_PCM_RATE_192K) {
		osr = OSR_MULTIPLE_2;
	} else if (cfg->i2s.frame_clk_freq >= AUDIO_PCM_RATE_96K) {
		osr = OSR_MULTIPLE_4;
	} else {
		osr = OSR_MULTIPLE_8;
	}

	LOG_INF("PCM Rate: %u OSR Multiple: %u", cfg->i2s.frame_clk_freq,
			osr);
	return osr;
}

static void codec_configure_output(const struct device *dev)
{
	const struct tlv320dac310x_config *const dev_cfg = dev->config;
	uint8_t val;

	/*
	 * set common mode voltage to 1.65V (half of AVDD)
	 * AVDD is typically 3.3V
	 */
	codec_read_reg(dev, HEADPHONE_DRV_ADDR, &val);
	val &= ~HEADPHONE_DRV_CM_MASK;
	val |= HEADPHONE_DRV_CM(CM_VOLTAGE_1P65) | HEADPHONE_DRV_RESERVED;
	codec_write_reg(dev, HEADPHONE_DRV_ADDR, val);

#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(use_volume_control_pin)
	if (dev_cfg->use_volume_control_pin) {
		/*
		 * set mic detect and volume control pin to volume control function.
		 * See chapter "6.3.10.2 DAC Digital-Volume Control" and chapter
		 * "6.3.10.3 Volume Control Pin" in the TLV320DAC3100 datasheet.
		 */
		codec_write_reg(dev, VOL_MICDET_ADC_CTRL_ADDR, VOL_MICDET_VOL_CTRL_PIN);
	}
#endif

	/* enable pop removal on power down/up */
	codec_read_reg(dev, HP_OUT_POP_RM_ADDR, &val);
	codec_write_reg(dev, HP_OUT_POP_RM_ADDR, val | HP_OUT_POP_RM_ENABLE);

	/* route DAC output to Mixer Amplifier -> so that volume control has effect */
	val = OUTPUT_ROUTING_MIXERL | OUTPUT_ROUTING_MIXERR;
	codec_write_reg(dev, OUTPUT_ROUTING_ADDR, val);

	/* enable volume control on Headphone out */
	codec_write_reg(dev, HPL_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(HPX_ANA_VOL_DEFAULT));
	codec_write_reg(dev, HPR_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(HPX_ANA_VOL_DEFAULT));

	/* set headphone outputs as line-out */
	codec_write_reg(dev, HEADPHONE_DRV_CTRL_ADDR, HEADPHONE_DRV_LINEOUT);

	/* unmute headphone drivers */
	codec_write_reg(dev, HPL_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE);
	codec_write_reg(dev, HPR_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE);

	/* power up headphone drivers */
	codec_read_reg(dev, HEADPHONE_DRV_ADDR, &val);
	val |= HEADPHONE_DRV_POWERUP | HEADPHONE_DRV_RESERVED;
	codec_write_reg(dev, HEADPHONE_DRV_ADDR, val);

	/* set speaker default volume */
	codec_write_reg(dev, SPKL_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(SPK_ANA_VOL_DEFAULT));
	codec_write_reg(dev, SPKR_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(SPK_ANA_VOL_DEFAULT));

	/* enable speaker */
	codec_read_reg(dev, SPEAKER_AMP_ADDR, &val);
	val |= SPEAKER_AMP_POWER;
	codec_write_reg(dev, SPEAKER_AMP_ADDR, val);

	/* unmute speaker and apply gain */
	switch (dev_cfg->speaker_gain) {
	case 6:
		val = SPEAKER_DRV_GAIN_6DB;
		break;
	case 12:
		val = SPEAKER_DRV_GAIN_12DB;
		break;
	case 18:
		val = SPEAKER_DRV_GAIN_18DB;
		break;
	case 24:
		val = SPEAKER_DRV_GAIN_24DB;
		break;
	default:
		LOG_ERR("Invalid speaker gain: %u dB. Using 6 dB", dev_cfg->speaker_gain);
		val = SPEAKER_DRV_GAIN_6DB;
		break;
	}
	val |= SPEAKER_DRV_UNMUTE;
	codec_write_reg(dev, SPKL_DRV_ADDR, val);
	codec_write_reg(dev, SPKR_DRV_ADDR, val);
}

static int codec_set_output_volume(const struct device *dev, int vol, audio_channel_t channel)
{
	uint8_t vol_val;
	int vol_index;
	uint8_t vol_array[] = {
		107, 108, 110, 113, 116, 120, 125, 128, 132, 138, 144
	};

	if ((vol > CODEC_OUTPUT_VOLUME_MAX) ||
			(vol < CODEC_OUTPUT_VOLUME_MIN)) {
		LOG_ERR("Invalid volume %d.%d dB",
				vol >> 1, ((uint32_t)vol & 1) ? 5 : 0);
		return -EINVAL;
	}

	/* remove sign */
	vol = -vol;

	/* if volume is near floor, set minimum */
	if (vol > HPX_ANA_VOL_FLOOR) {
		vol_val = HPX_ANA_VOL_FLOOR;
	} else if (vol > HPX_ANA_VOL_LOW_THRESH) {
		/* lookup low volume values */
		for (vol_index = 0; vol_index < ARRAY_SIZE(vol_array); vol_index++) {
			if (vol_array[vol_index] >= vol) {
				break;
			}
		}
		vol_val = HPX_ANA_VOL_LOW_THRESH + vol_index + 1;
	} else {
		vol_val = (uint8_t)vol;
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_HEADPHONE_LEFT)) {
		codec_write_reg(dev, HPL_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(vol_val));
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_HEADPHONE_RIGHT)) {
		codec_write_reg(dev, HPR_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(vol_val));
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_FRONT_LEFT) ||
	    (channel == AUDIO_CHANNEL_REAR_LEFT) ||
	    (channel == AUDIO_CHANNEL_SIDE_LEFT)) {
		codec_write_reg(dev, SPKL_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(vol_val));
	}

	if ((channel == AUDIO_CHANNEL_ALL) ||
	    (channel == AUDIO_CHANNEL_FRONT_RIGHT) ||
	    (channel == AUDIO_CHANNEL_REAR_RIGHT) ||
	    (channel == AUDIO_CHANNEL_SIDE_RIGHT)) {
		codec_write_reg(dev, SPKR_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(vol_val));
	}

	return 0;
}

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
static void codec_read_all_regs(const struct device *dev)
{
	uint8_t val;

	codec_read_reg(dev, SOFT_RESET_ADDR, &val);
	codec_read_reg(dev, NDAC_DIV_ADDR, &val);
	codec_read_reg(dev, MDAC_DIV_ADDR, &val);
	codec_read_reg(dev, OSR_MSB_ADDR, &val);
	codec_read_reg(dev, OSR_LSB_ADDR, &val);
	codec_read_reg(dev, IF_CTRL1_ADDR, &val);
	codec_read_reg(dev, BCLK_DIV_ADDR, &val);
	codec_read_reg(dev, OVF_FLAG_ADDR, &val);
	codec_read_reg(dev, PROC_BLK_SEL_ADDR, &val);
	codec_read_reg(dev, DATA_PATH_SETUP_ADDR, &val);
	codec_read_reg(dev, VOL_CTRL_ADDR, &val);
	codec_read_reg(dev, L_DIG_VOL_CTRL_ADDR, &val);
	codec_read_reg(dev, DRC_CTRL1_ADDR, &val);
	codec_read_reg(dev, L_BEEP_GEN_ADDR, &val);
	codec_read_reg(dev, R_BEEP_GEN_ADDR, &val);
	codec_read_reg(dev, BEEP_LEN_MSB_ADDR, &val);
	codec_read_reg(dev, BEEP_LEN_MIB_ADDR, &val);
	codec_read_reg(dev, BEEP_LEN_LSB_ADDR, &val);

	codec_read_reg(dev, HEADPHONE_DRV_ADDR, &val);
	codec_read_reg(dev, HP_OUT_POP_RM_ADDR, &val);
	codec_read_reg(dev, OUTPUT_ROUTING_ADDR, &val);
	codec_read_reg(dev, HPL_ANA_VOL_CTRL_ADDR, &val);
	codec_read_reg(dev, HPR_ANA_VOL_CTRL_ADDR, &val);
	codec_read_reg(dev, HPL_DRV_GAIN_CTRL_ADDR, &val);
	codec_read_reg(dev, HPR_DRV_GAIN_CTRL_ADDR, &val);
	codec_read_reg(dev, HEADPHONE_DRV_CTRL_ADDR, &val);

	codec_read_reg(dev, TIMER_MCLK_DIV_ADDR, &val);
}
#endif

static DEVICE_API(audio_codec, codec_driver_api) = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
};

#define TLV320DAC31X_DEFINE(inst)                                                                  \
	static struct tlv320dac310x_data codec_device_data_##inst;                                 \
                                                                                                   \
	static struct tlv320dac310x_config codec_device_config_##inst = {                          \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.speaker_gain = DT_INST_PROP(inst, speaker_gain),                                  \
		.use_volume_control_pin = (bool)DT_INST_PROP_OR(inst, use_volume_control_pin, 0),  \
		.use_internal_pll_with_bclk =                                                      \
			(bool)DT_INST_PROP_OR(inst, use_internal_pll_with_bclk, 0),                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, codec_initialize, NULL, &codec_device_data_##inst,             \
			      &codec_device_config_##inst, POST_KERNEL,                            \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLV320DAC31X_DEFINE)
