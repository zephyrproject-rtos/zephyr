/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nau8540

#include <errno.h>
#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "nau8540.h"

LOG_MODULE_REGISTER(nau8540, CONFIG_AUDIO_CODEC_LOG_LEVEL);

struct nau8540_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec csb_gpio;
	uint8_t clock_source;
	uint8_t adc_channels;
};

struct nau8540_data {
	uint8_t adc_channels;
	uint16_t mute;
	uint32_t frame_clk_freq;
};

struct nau8540_pcm {
	audio_dai_type_t dai_type;
	uint8_t channels;
	uint8_t word_bits;
	uint32_t fs;
	uint8_t options;
};

struct nau8540_fll_params {
	uint8_t mclk_reg;
	uint8_t ratio;
	uint16_t fll_int;
	uint16_t fll_frac;
};

struct nau8540_mclk_div {
	uint8_t div;
	uint8_t reg;
};

static const struct nau8540_mclk_div nau8540_mclk_divs[] = {
	{1, 0x0}, {2, 0x2}, {3, 0x7}, {4, 0x3}, {6, 0xa},
	{8, 0x4}, {12, 0xb}, {16, 0x5}, {24, 0xc}, {32, 0x6},
};

static int nau8540_write_reg(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct nau8540_config *cfg = dev->config;
	uint8_t buf[4] = {
		(uint8_t)(reg >> 8),
		(uint8_t)(reg & 0xff),
		(uint8_t)(val >> 8),
		(uint8_t)(val & 0xff),
	};
	int ret;

	ret = i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("I2C write failed at 0x%02x (%d)", reg, ret);
		return ret;
	}

	LOG_DBG("REG 0x%02x = 0x%04x", reg, val);
	return 0;
}

static int nau8540_read_reg(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct nau8540_config *cfg = dev->config;
	uint8_t addr[2] = {
		(uint8_t)(reg >> 8),
		(uint8_t)(reg & 0xff),
	};
	uint8_t data[2];
	int ret;

	ret = i2c_write_read_dt(&cfg->i2c, addr, sizeof(addr), data, sizeof(data));
	if (ret < 0) {
		LOG_ERR("I2C read failed at 0x%02x (%d)", reg, ret);
		return ret;
	}

	*val = ((uint16_t)data[0] << 8) | data[1];
	return 0;
}

static int nau8540_update_reg(const struct device *dev, uint16_t reg, uint16_t mask, uint16_t val)
{
	uint16_t cur;
	int ret;

	ret = nau8540_read_reg(dev, reg, &cur);
	if (ret < 0) {
		return ret;
	}

	return nau8540_write_reg(dev, reg, (cur & ~mask) | (val & mask));
}

static int nau8540_soft_reset(const struct device *dev)
{
	int ret;

	ret = nau8540_write_reg(dev, NAU8540_REG_SW_RESET, 0x0000);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_SW_RESET, 0x0000);
	if (ret < 0) {
		return ret;
	}

	k_msleep(2);
	return 0;
}

/* FLL1.FLL_RATIO[6:0] is one-hot. Thresholds are FREF in Hz. */
static int nau8540_fll_ratio(uint32_t fref)
{
	if (fref >= 512000U) {
		return 1;
	}
	if (fref >= 256000U) {
		return 2;
	}
	if (fref >= 128000U) {
		return 4;
	}
	if (fref >= 64000U) {
		return 8;
	}
	if (fref >= 32000U) {
		return 16;
	}
	if (fref >= 8000U) {
		return 32;
	}
	if (fref >= 4000U) {
		return 64;
	}

	return -EINVAL;
}

/*
 * CLK_ADC = OSR * Fs and must stay at or below 6.144 MHz.
 * MCLK is 256 * Fs, so CLK_ADC_SRC divides by 256 / OSR.
 */
static int nau8540_adc_clock(uint32_t fs, uint16_t *osr, uint16_t *adc_src)
{
	if ((fs == 0U) || (fs > (NAU8540_CLK_ADC_MAX_HZ / 32U))) {
		return -EINVAL;
	}

	if (fs <= (NAU8540_CLK_ADC_MAX_HZ / 256U)) {
		*osr = NAU8540_ADC_OSR_256;
		*adc_src = NAU8540_CLK_ADC_SRC_DIV1;
	} else if (fs <= (NAU8540_CLK_ADC_MAX_HZ / 128U)) {
		*osr = NAU8540_ADC_OSR_128;
		*adc_src = NAU8540_CLK_ADC_SRC_DIV2;
	} else if (fs <= (NAU8540_CLK_ADC_MAX_HZ / 64U)) {
		*osr = NAU8540_ADC_OSR_64;
		*adc_src = NAU8540_CLK_ADC_SRC_DIV4;
	} else {
		*osr = NAU8540_ADC_OSR_32;
		*adc_src = NAU8540_CLK_ADC_SRC_DIV8;
	}

	return 0;
}

/*
 * FDCO = FREF * FLL_RATIO * FLL_INTEGER, kept in 90-100 MHz.
 * FLL_RATIO is the one-hot prescaler. Dividing it out is what lets a
 * 48 kHz FS reference fit the 10-bit integer (2048 / 16 = 128). The
 * datasheet's 12 MHz example uses ratio 1, so the published equation
 * shows the unscaled quotient.
 */
static int nau8540_calc_fll(uint32_t fs, struct nau8540_fll_params *out)
{
	uint32_t best_err = UINT32_MAX;
	bool found = false;
	int ratio;
	size_t i;

	ratio = nau8540_fll_ratio(fs);
	if (ratio < 0) {
		LOG_ERR("FLL reference %u Hz is below 4 kHz", fs);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(nau8540_mclk_divs); i++) {
		uint64_t fdco = (uint64_t)fs * 512ULL * nau8540_mclk_divs[i].div;
		uint32_t num = 512U * nau8540_mclk_divs[i].div;
		uint32_t fll_int = num / (uint32_t)ratio;
		uint32_t err;

		if ((fdco < 90000000ULL) || (fdco > 100000000ULL)) {
			continue;
		}
		if ((fll_int == 0U) || (fll_int > NAU8540_FLL_INTEGER_MASK)) {
			continue;
		}

		err = (fdco > 95000000ULL) ? (uint32_t)(fdco - 95000000ULL)
					   : (uint32_t)(95000000ULL - fdco);
		if (err < best_err) {
			best_err = err;
			out->mclk_reg = nau8540_mclk_divs[i].reg;
			out->ratio = (uint8_t)ratio;
			out->fll_int = (uint16_t)fll_int;
			out->fll_frac = (uint16_t)(((num % (uint32_t)ratio) * 65536U) /
						   (uint32_t)ratio);
			found = true;
		}
	}

	if (!found) {
		LOG_ERR("No FLL setting keeps FDCO in 90-100 MHz for %u Hz", fs);
		return -EINVAL;
	}

	return 0;
}

/* Program the FLL while SYSCLK is still MCLK, then switch SYSCLK to the VCO. */
static int nau8540_apply_fll(const struct device *dev, const struct nau8540_fll_params *fll,
			     uint16_t adc_src)
{
	int ret;

	ret = nau8540_write_reg(dev, NAU8540_REG_CLOCK_SRC, adc_src | fll->mclk_reg);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_FLL_VCO_RSV, NAU8540_FLL_VCO_RSV_DCO);
	if (ret < 0) {
		return ret;
	}

	/* Linux: ICTRL_LATCH=6. RAK omits it; FS lock is more reliable with it. */
	ret = nau8540_write_reg(dev, NAU8540_REG_FLL1,
				(fll->ratio & NAU8540_FLL_RATIO_MASK) | NAU8540_ICTRL_LATCH_6);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_FLL2, fll->fll_frac);
	if (ret < 0) {
		return ret;
	}

	/* Linux: FS is a slow FLL ref and needs GAIN_ERR=0xf or the VCO stays dead. */
	ret = nau8540_write_reg(dev, NAU8540_REG_FLL3,
				NAU8540_FLL_CLK_SRC_FS | NAU8540_GAIN_ERR_FS |
					(fll->fll_int & NAU8540_FLL_INTEGER_MASK));
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_FLL4, 0x0010);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_FLL5, 0xc000);
	if (ret < 0) {
		return ret;
	}

	/*
	 * 0x6000 is the reset value (SDM and 500 kHz cutoff). A non-zero
	 * fraction also requires FLL6.SDM_EN.
	 */
	ret = nau8540_write_reg(dev, NAU8540_REG_FLL6,
				0x6000 | (fll->fll_frac != 0U ? NAU8540_SDM_EN : 0));

	if (ret < 0) {
		return ret;
	}

	k_msleep(2);
	return nau8540_update_reg(dev, NAU8540_REG_CLOCK_SRC, NAU8540_CLK_SRC_VCO,
				  NAU8540_CLK_SRC_VCO);
}

/* Master-mode BCLK_DIV and LRC_DIV from BCLK = Fs * word_bits * channels. */
static int nau8540_master_clocks(uint8_t word_bits, uint8_t channels, uint16_t *divs)
{
	switch ((uint16_t)word_bits * channels) {
	case 32:
		*divs = NAU8540_I2S_LRC_DIV_32 | NAU8540_I2S_BCLK_DIV_8;
		return 0;
	case 64:
		*divs = NAU8540_I2S_LRC_DIV_64 | NAU8540_I2S_BCLK_DIV_4;
		return 0;
	case 128:
		*divs = NAU8540_I2S_LRC_DIV_128 | NAU8540_I2S_BCLK_DIV_2;
		return 0;
	case 256:
		*divs = NAU8540_I2S_LRC_DIV_256 | NAU8540_I2S_BCLK_DIV_1;
		return 0;
	default:
		return -EINVAL;
	}
}

static int nau8540_word_length(uint8_t word_bits, uint16_t *dl)
{
	switch (word_bits) {
	case AUDIO_PCM_WIDTH_16_BITS:
		*dl = NAU8540_I2S_DL_16;
		return 0;
	case AUDIO_PCM_WIDTH_20_BITS:
		*dl = NAU8540_I2S_DL_20;
		return 0;
	case AUDIO_PCM_WIDTH_24_BITS:
		*dl = NAU8540_I2S_DL_24;
		return 0;
	case AUDIO_PCM_WIDTH_32_BITS:
		*dl = NAU8540_I2S_DL_32;
		return 0;
	default:
		return -EINVAL;
	}
}

static int nau8540_configure_dai(const struct device *dev, const struct nau8540_pcm *stream,
				 bool drives_clocks)
{
	uint16_t pcm0 = 0;
	uint16_t pcm1 = NAU8540_I2S_DO12_DRV | NAU8540_I2S_DO12_OE;
	uint16_t divs = NAU8540_I2S_LRC_DIV_32 | NAU8540_I2S_BCLK_DIV_8;
	uint16_t dl;
	int ret;

	switch (stream->dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		pcm0 |= NAU8540_I2S_DF_I2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		pcm0 |= NAU8540_I2S_DF_LEFT;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		pcm0 |= NAU8540_I2S_DF_RIGHT;
		break;
	case AUDIO_DAI_TYPE_PCMA:
	case AUDIO_DAI_TYPE_PCM:
		/* PCM A: AIFMT = 11, LRP = 0. */
		pcm0 |= NAU8540_I2S_DF_PCM_AB;
		break;
	case AUDIO_DAI_TYPE_PCMB:
		/* PCM B: AIFMT = 11, LRP = 1. */
		pcm0 |= NAU8540_I2S_DF_PCM_AB | NAU8540_I2S_LRP;
		break;
	default:
		LOG_ERR("Unsupported DAI type %d", stream->dai_type);
		return -ENOTSUP;
	}

	ret = nau8540_word_length(stream->word_bits, &dl);
	if (ret < 0) {
		LOG_ERR("Unsupported word size %u", stream->word_bits);
		return ret;
	}
	pcm0 |= dl;

	if (nau8540_master_clocks(stream->word_bits, stream->channels, &divs) < 0) {
		/* Slave clocks come from the host. Keep a legal divider pair. */
		divs = NAU8540_I2S_LRC_DIV_32 | NAU8540_I2S_BCLK_DIV_8;
	}
	pcm1 |= divs;
	if (drives_clocks) {
		pcm1 |= NAU8540_I2S_MS_MASTER;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_PCM_CTRL0, pcm0);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_PCM_CTRL1, pcm1);
	if (ret < 0) {
		return ret;
	}

	/* RAK NAU85L40B.c PCM_CTRL2: DO34_DRV and DO34_OE. */
	return nau8540_write_reg(dev, NAU8540_REG_PCM_CTRL2, 0x4800);
}

static int nau8540_init_analog(const struct device *dev, uint8_t channels, uint16_t osr)
{
	int ret;

	ret = nau8540_write_reg(dev, NAU8540_REG_VMID_CTRL,
				NAU8540_VMID_EN | (0x2 << NAU8540_VMID_SEL_SFT));
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_REFERENCE,
				NAU8540_DISCHRG_EN | NAU8540_PRECHARGE_DIS |
					NAU8540_GLOBAL_BIAS_EN);
	if (ret < 0) {
		return ret;
	}

	/* Charge MIC coupling caps (Linux nau8540_precharge_event). */
	k_msleep(40);
	ret = nau8540_update_reg(dev, NAU8540_REG_REFERENCE, NAU8540_DISCHRG_EN, 0);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_MIC_BIAS, 0x0d04 | NAU8540_PU_PRE);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_CLOCK_CTRL,
				NAU8540_CLK_ADC_EN | NAU8540_CLK_AGC_EN | NAU8540_CLK_I2S_EN);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_ADC_SAMPLE_RATE, osr);
	if (ret < 0) {
		return ret;
	}

	/* MODE_CH[3] shorts MICP/MICN to ground. Use the 16 kHz AA bit only. */
	ret = nau8540_write_reg(dev, NAU8540_REG_FEPGA1, NAU8540_FEPGA_AA_16KHZ);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_FEPGA2, NAU8540_FEPGA_AA_16KHZ);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_FEPGA3, 0x0101);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_FEPGA4, 0x0101);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_ANALOG_ADC1, 0x0011);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_ANALOG_ADC2, 0x0020);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_ANALOG_PWR, channels);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_PWR, 0xf000);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_POWER_MANAGEMENT, channels);
	if (ret < 0) {
		return ret;
	}

	/* PCM_CTRL4.ADC_TXEN[3:0] follows the powered channels. */
	ret = nau8540_write_reg(dev, NAU8540_REG_PCM_CTRL4, channels);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_DIGITAL_MUX, 0x00e4);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_HPF_FILTER_CH12, 0x1f1f);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_HPF_FILTER_CH34, 0x1f1f);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_ALC_CONTROL_3,
				NAU8540_ALC_CH_ALL_EN | NAU8540_ALC_CONTROL_3_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_write_reg(dev, NAU8540_REG_MUTE, 0x0000);
	if (ret < 0) {
		return ret;
	}

	return nau8540_write_reg(dev, NAU8540_REG_I2C_CTRL, 0xefff);
}

static int nau8540_parse_stream(const struct nau8540_config *dev_cfg,
				const struct audio_codec_cfg *cfg, struct nau8540_pcm *out)
{
	switch (cfg->dai_type) {
	case AUDIO_DAI_TYPE_I2S:
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		out->channels = cfg->dai_cfg.i2s.channels;
		out->word_bits = cfg->dai_cfg.i2s.word_size;
		out->fs = cfg->dai_cfg.i2s.frame_clk_freq;
		out->options = cfg->dai_cfg.i2s.options;
		break;
	case AUDIO_DAI_TYPE_PCMA:
	case AUDIO_DAI_TYPE_PCMB:
	case AUDIO_DAI_TYPE_PCM:
		out->channels = cfg->dai_cfg.pcm.channels;
		out->word_bits = (uint8_t)cfg->dai_cfg.pcm.pcm_width;
		out->fs = (uint32_t)cfg->dai_cfg.pcm.samplerate;
		/*
		 * pcm_config has no clock-role bits. FLL-FS needs an external
		 * FS, so the codec is the frame-clock target. Free-running
		 * DCO has to drive BCLK and FS.
		 */
		out->options = (dev_cfg->clock_source == NAU8540_CLK_SRC_INTERNAL)
				       ? 0U
				       : I2S_OPT_FRAME_CLK_TARGET;
		break;
	default:
		LOG_ERR("Unsupported DAI type %d", cfg->dai_type);
		return -ENOTSUP;
	}

	out->dai_type = cfg->dai_type;
	return 0;
}

static int nau8540_adc_mask(uint8_t count, uint8_t dt_mask, uint8_t *mask)
{
	uint8_t need;

	if ((count == 0U) || (count > 4U)) {
		return -EINVAL;
	}

	need = (uint8_t)BIT_MASK(count);
	if ((need & dt_mask) != need) {
		LOG_ERR("Requested %u channels, adc-channels mask is 0x%x", count, dt_mask);
		return -EINVAL;
	}

	*mask = need;
	return 0;
}

static int nau8540_configure_fll(const struct device *dev, uint32_t fs)
{
	struct nau8540_fll_params fll;
	uint16_t osr;
	uint16_t adc_src;
	int ret;

	ret = nau8540_adc_clock(fs, &osr, &adc_src);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_calc_fll(fs, &fll);
	if (ret < 0) {
		return ret;
	}

	return nau8540_apply_fll(dev, &fll, adc_src);
}

static int nau8540_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct nau8540_config *dev_cfg = dev->config;
	struct nau8540_data *data = dev->data;
	struct nau8540_pcm stream;
	struct nau8540_fll_params fll = {0};
	uint8_t channels;
	uint16_t osr = NAU8540_ADC_OSR_128;
	uint16_t adc_src = NAU8540_CLK_ADC_SRC_DIV2;
	uint16_t dl;
	bool drives_clocks;
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}

	if (cfg->dai_route == AUDIO_ROUTE_BYPASS) {
		ret = nau8540_soft_reset(dev);
		if (ret < 0) {
			return ret;
		}

		data->adc_channels = 0U;
		data->mute = 0U;
		data->frame_clk_freq = 0U;
		return 0;
	}

	if ((cfg->dai_route != AUDIO_ROUTE_CAPTURE) &&
	    (cfg->dai_route != AUDIO_ROUTE_PLAYBACK_CAPTURE)) {
		LOG_ERR("NAU85L40B is a capture ADC");
		return -ENOTSUP;
	}

	ret = nau8540_parse_stream(dev_cfg, cfg, &stream);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_adc_mask(stream.channels, dev_cfg->adc_channels, &channels);
	if (ret < 0) {
		return ret;
	}

	if (nau8540_word_length(stream.word_bits, &dl) < 0) {
		LOG_ERR("Unsupported word size %u", stream.word_bits);
		return -EINVAL;
	}
	ARG_UNUSED(dl);

	if (dev_cfg->clock_source == NAU8540_CLK_SRC_INTERNAL) {
		/* Datasheet: free-running FLL requires PCM_CTRL1.MS = 1. */
		if ((stream.options & I2S_OPT_FRAME_CLK_TARGET) != 0U) {
			LOG_WRN("Free-running DCO drives BCLK/FS");
		}
		drives_clocks = true;
	} else {
		if ((stream.options & I2S_OPT_FRAME_CLK_TARGET) == 0U) {
			LOG_ERR("FLL locked to FS requires an external frame clock");
			return -EINVAL;
		}
		drives_clocks = false;
		ret = nau8540_adc_clock(stream.fs, &osr, &adc_src);
		if (ret < 0) {
			LOG_ERR("Sample rate %u Hz exceeds CLK_ADC max", stream.fs);
			return ret;
		}
		ret = nau8540_calc_fll(stream.fs, &fll);
		if (ret < 0) {
			return ret;
		}
	}

	if (drives_clocks) {
		uint16_t divs;

		if (nau8540_master_clocks(stream.word_bits, stream.channels, &divs) < 0) {
			LOG_ERR("Master dividers do not support %u-bit %u channels",
				stream.word_bits, stream.channels);
			return -EINVAL;
		}
	}

	ret = nau8540_soft_reset(dev);
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_init_analog(dev, channels, osr);
	if (ret < 0) {
		return ret;
	}

	if (dev_cfg->clock_source == NAU8540_CLK_SRC_INTERNAL) {
		/*
		 * Free-running DCO. FLL_VCO_RSV = 0xF13C and MCLK_SRC = 1/4
		 * target MCLK = 256 * 48 kHz. CLK_ADC = MCLK / 2 with OSR 128.
		 */
		ret = nau8540_write_reg(dev, NAU8540_REG_FLL_VCO_RSV, NAU8540_FLL_VCO_RSV_DCO);
		if (ret < 0) {
			return ret;
		}
		ret = nau8540_write_reg(dev, NAU8540_REG_FLL6, NAU8540_DCO_EN);
		if (ret < 0) {
			return ret;
		}
		k_msleep(2);
		ret = nau8540_write_reg(dev, NAU8540_REG_CLOCK_SRC,
					NAU8540_CLK_SRC_VCO | adc_src | NAU8540_MCLK_SRC_DIV4);
	} else {
		ret = nau8540_apply_fll(dev, &fll, adc_src);
	}
	if (ret < 0) {
		return ret;
	}

	ret = nau8540_configure_dai(dev, &stream, drives_clocks);
	if (ret < 0) {
		return ret;
	}

	data->adc_channels = channels;
	data->mute = 0U;
	data->frame_clk_freq = stream.fs;
	return 0;
}

static void nau8540_start_output(const struct device *dev)
{
	const struct nau8540_config *dev_cfg = dev->config;
	struct nau8540_data *data = dev->data;

	/*
	 * Host I2S must already be running. Relock FLL to FS now; doing this
	 * during configure() leaves VCO selected with no reference.
	 */
	if ((dev_cfg->clock_source != NAU8540_CLK_SRC_INTERNAL) &&
	    (data->frame_clk_freq != 0U)) {
		(void)nau8540_configure_fll(dev, data->frame_clk_freq);
	}

	(void)nau8540_update_reg(dev, NAU8540_REG_PCM_CTRL1, NAU8540_I2S_DO12_TRI, 0);
	(void)nau8540_update_reg(dev, NAU8540_REG_PCM_CTRL1, NAU8540_I2S_DO12_OE,
				 NAU8540_I2S_DO12_OE);
	(void)nau8540_write_reg(dev, NAU8540_REG_POWER_MANAGEMENT, data->adc_channels);
	(void)nau8540_write_reg(dev, NAU8540_REG_FEPGA1, NAU8540_FEPGA_AA_16KHZ);
	(void)nau8540_write_reg(dev, NAU8540_REG_FEPGA2, NAU8540_FEPGA_AA_16KHZ);

	/* Linux recovery: mute/unmute and pulse RST if the ADC path is stuck. */
	(void)nau8540_write_reg(dev, NAU8540_REG_MUTE, NAU8540_PGA_CH_ALL_MUTE);
	(void)nau8540_write_reg(dev, NAU8540_REG_MUTE, 0);
	(void)nau8540_write_reg(dev, NAU8540_REG_RST, 0x0001);
	(void)nau8540_write_reg(dev, NAU8540_REG_RST, 0x0000);
	(void)nau8540_write_reg(dev, NAU8540_REG_MUTE, data->mute);
	(void)nau8540_write_reg(dev, NAU8540_REG_POWER_MANAGEMENT, data->adc_channels);
}

static void nau8540_stop_output(const struct device *dev)
{
	(void)nau8540_update_reg(dev, NAU8540_REG_PCM_CTRL1, NAU8540_I2S_DO12_TRI,
				 NAU8540_I2S_DO12_TRI);
	(void)nau8540_write_reg(dev, NAU8540_REG_POWER_MANAGEMENT, 0);
}

static int nau8540_set_gain(const struct device *dev, audio_channel_t channel, int volume_db)
{
	uint16_t steps;
	uint16_t gain;
	int ret = 0;

	if (volume_db < 0) {
		volume_db = 0;
	}
	if (volume_db > 36) {
		volume_db = 36;
	}

	/* Digital gain steps of 0.125 dB. RAK uses a channel tag in [15:12]. */
	steps = (uint16_t)(volume_db * 8);
	if (steps > NAU8540_DIGITAL_GAIN_MAX_STEPS) {
		steps = NAU8540_DIGITAL_GAIN_MAX_STEPS;
	}
	gain = steps;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		return nau8540_write_reg(dev, NAU8540_REG_DIGITAL_GAIN_CH1,
					 NAU8540_DIGITAL_GAIN_CH1_0DB + gain);
	case AUDIO_CHANNEL_FRONT_RIGHT:
		return nau8540_write_reg(dev, NAU8540_REG_DIGITAL_GAIN_CH2,
					 NAU8540_DIGITAL_GAIN_CH2_0DB + gain);
	case AUDIO_CHANNEL_ALL:
		ret = nau8540_write_reg(dev, NAU8540_REG_DIGITAL_GAIN_CH1,
					NAU8540_DIGITAL_GAIN_CH1_0DB + gain);
		if (ret < 0) {
			return ret;
		}
		ret = nau8540_write_reg(dev, NAU8540_REG_DIGITAL_GAIN_CH2,
					NAU8540_DIGITAL_GAIN_CH2_0DB + gain);
		if (ret < 0) {
			return ret;
		}
		ret = nau8540_write_reg(dev, NAU8540_REG_DIGITAL_GAIN_CH3,
					NAU8540_DIGITAL_GAIN_CH3_0DB + gain);
		if (ret < 0) {
			return ret;
		}
		return nau8540_write_reg(dev, NAU8540_REG_DIGITAL_GAIN_CH4,
					 NAU8540_DIGITAL_GAIN_CH4_0DB + gain);
	default:
		return -EINVAL;
	}
}

static int nau8540_set_mute(const struct device *dev, audio_channel_t channel, bool mute)
{
	struct nau8540_data *data = dev->data;
	uint16_t mask;
	int ret;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		mask = BIT(0);
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		mask = BIT(1);
		break;
	case AUDIO_CHANNEL_ALL:
		mask = NAU8540_PGA_CH_ALL_MUTE;
		break;
	default:
		return -EINVAL;
	}

	ret = nau8540_update_reg(dev, NAU8540_REG_MUTE, mask, mute ? mask : 0);
	if (ret < 0) {
		return ret;
	}

	if (mute) {
		data->mute |= mask;
	} else {
		data->mute &= ~mask;
	}

	return 0;
}

static int nau8540_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_INPUT_VOLUME:
		return nau8540_set_gain(dev, channel, val.vol);
	case AUDIO_PROPERTY_INPUT_MUTE:
		return nau8540_set_mute(dev, channel, val.mute);
	default:
		return -ENOTSUP;
	}
}

static int nau8540_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static DEVICE_API(audio_codec, nau8540_driver_api) = {
	.configure = nau8540_configure,
	.start_output = nau8540_start_output,
	.stop_output = nau8540_stop_output,
	.set_property = nau8540_set_property,
	.apply_properties = nau8540_apply_properties,
};

static int nau8540_init(const struct device *dev)
{
	const struct nau8540_config *cfg = dev->config;
	uint16_t device_id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	if (cfg->csb_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->csb_gpio)) {
			LOG_ERR("CSB GPIO not ready");
			return -ENODEV;
		}

		/*
		 * CSB is a physical strap: low selects 0x1c, high selects 0x1d.
		 * GPIO_OUTPUT_HIGH/LOW stay at that level when the specifier
		 * has GPIO_ACTIVE_LOW. On RAK18040 this pin is also MIC_CTR;
		 * driving it low kills the analog mic path.
		 */
		ret = gpio_pin_configure_dt(&cfg->csb_gpio,
					    ((cfg->i2c.addr & BIT(0)) != 0U) ? GPIO_OUTPUT_HIGH
									     : GPIO_OUTPUT_LOW);
		if (ret < 0) {
			LOG_ERR("Failed to configure CSB GPIO (%d)", ret);
			return ret;
		}

		k_msleep(2);
	}

	ret = nau8540_read_reg(dev, NAU8540_REG_I2C_DEVICE_ID, &device_id);
	if (ret < 0) {
		LOG_ERR("Failed to read NAU85L40B device ID");
		return -ENODEV;
	}

	LOG_INF("NAU85L40B device ID 0x%04x at 0x%02x", device_id, cfg->i2c.addr);
	return 0;
}

#define NAU8540_INIT(n)                                                                            \
	static struct nau8540_data nau8540_data_##n;                                               \
	static const struct nau8540_config nau8540_config_##n = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.csb_gpio = GPIO_DT_SPEC_INST_GET_OR(n, csb_gpios, {0}),                           \
		.clock_source = DT_INST_ENUM_IDX(n, clock_source),                                 \
		.adc_channels = DT_INST_PROP(n, adc_channels),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, nau8540_init, NULL, &nau8540_data_##n, &nau8540_config_##n,       \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &nau8540_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NAU8540_INIT)
