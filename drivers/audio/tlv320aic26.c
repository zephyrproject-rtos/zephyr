/*
 * Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT ti_tlv320aic26

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/audio/codec.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "tlv320aic26.h"

LOG_MODULE_REGISTER(tlv320aic26, CONFIG_AUDIO_CODEC_LOG_LEVEL);

struct tlv320aic26_config {
	struct spi_dt_spec spi;
	uint32_t mclk_freq;
};

struct tlv320aic26_data {
	audio_route_t route;
	bool configured;
};

struct aic26_pll_cfg {
	uint16_t d;
	uint8_t p;
	uint8_t j;
	uint8_t q;
	uint8_t fs_div;
	bool pll_en;
	bool fsref_44k1;
};

static int aic26_reg_write(const struct device *dev, uint8_t page,
			   uint8_t addr, uint16_t val)
{
	const struct tlv320aic26_config *cfg = dev->config;
	uint16_t cmd;
	uint8_t buf[4];
	const struct spi_buf tx = { .buf = buf, .len = sizeof(buf) };
	const struct spi_buf_set tx_set = { .buffers = &tx, .count = 1 };

	cmd = FIELD_PREP(AIC26_CMD_PAGE, page) |
	      FIELD_PREP(AIC26_CMD_ADDR, addr);

	sys_put_be16(cmd, &buf[0]);
	sys_put_be16(val, &buf[2]);

	return spi_write_dt(&cfg->spi, &tx_set);
}

static int aic26_reg_read(const struct device *dev, uint8_t page,
			  uint8_t addr, uint16_t *val)
{
	const struct tlv320aic26_config *cfg = dev->config;
	uint16_t cmd;
	uint8_t tx_buf[4] = {0};
	uint8_t rx_buf[4] = {0};
	const struct spi_buf tx = { .buf = tx_buf, .len = sizeof(tx_buf) };
	const struct spi_buf_set tx_set = { .buffers = &tx, .count = 1 };
	const struct spi_buf rx = { .buf = rx_buf, .len = sizeof(rx_buf) };
	const struct spi_buf_set rx_set = { .buffers = &rx, .count = 1 };
	int ret;

	cmd = AIC26_CMD_READ |
	      FIELD_PREP(AIC26_CMD_PAGE, page) |
	      FIELD_PREP(AIC26_CMD_ADDR, addr);

	sys_put_be16(cmd, &tx_buf[0]);

	ret = spi_transceive_dt(&cfg->spi, &tx_set, &rx_set);
	if (ret == 0) {
		*val = sys_get_be16(&rx_buf[2]);
	}

	return ret;
}

static int aic26_reg_update(const struct device *dev, uint8_t page,
			    uint8_t addr, uint16_t mask, uint16_t val)
{
	uint16_t reg;
	int ret;

	ret = aic26_reg_read(dev, page, addr, &reg);
	if (ret < 0) {
		return ret;
	}

	return aic26_reg_write(dev, page, addr, (reg & ~mask) | (val & mask));
}

static int aic26_calc_pll(uint32_t mclk, uint32_t fs,
			  struct aic26_pll_cfg *out)
{
	static const uint32_t fsref_tbl[] = { 48000, 44100 };
	uint64_t num;
	uint64_t k10k;
	uint64_t vco_hz;
	uint32_t fsref;
	uint32_t mclk_p;
	uint32_t div_x2;
	uint32_t q;
	uint32_t j;
	uint32_t d;
	int8_t fs_div;
	int fi;
	uint8_t p;

	memset(out, 0, sizeof(*out));

	if (fs == 0 || mclk == 0) {
		return -EINVAL;
	}

	for (fi = 0; fi < ARRAY_SIZE(fsref_tbl); fi++) {
		fsref = fsref_tbl[fi];

		if ((fsref * 2) % fs != 0) {
			continue;
		}

		div_x2 = (fsref * 2) / fs;

		switch (div_x2) {
		case 2:
			fs_div = AIC26_FS_DIV_1;
			break;
		case 3:
			fs_div = AIC26_FS_DIV_1_5;
			break;
		case 4:
			fs_div = AIC26_FS_DIV_2;
			break;
		case 6:
			fs_div = AIC26_FS_DIV_3;
			break;
		case 8:
			fs_div = AIC26_FS_DIV_4;
			break;
		case 10:
			fs_div = AIC26_FS_DIV_5;
			break;
		case 11:
			fs_div = AIC26_FS_DIV_5_5;
			break;
		case 12:
			fs_div = AIC26_FS_DIV_6;
			break;
		default:
			continue;
		}

		out->fs_div = fs_div;
		out->fsref_44k1 = (fi == 1);

		if ((mclk % (128U * fsref)) == 0) {
			q = mclk / (128U * fsref);

			if (q >= 2 && q <= 17) {
				out->pll_en = false;
				out->q = q;
				return 0;
			}
		}

		for (p = 1; p <= 8; p++) {
			mclk_p = mclk / p;

			if (mclk_p < 2000000 || mclk_p > 20000000) {
				continue;
			}

			num = (uint64_t)fsref * 2048 * p * 10000ULL;
			k10k = num / mclk;
			j = (uint32_t)(k10k / 10000);
			d = (uint32_t)(k10k % 10000);

			if (d != 0) {
				if (j < 4 || j > 11 || mclk_p < 10000000) {
					continue;
				}
			} else {
				if (j < 4 || j > 55) {
					continue;
				}
			}

			if ((uint64_t)mclk * k10k != num) {
				continue;
			}

			vco_hz = ((uint64_t)mclk * k10k) /
				 ((uint64_t)p * 10000);

			if (vco_hz < 80000000ULL || vco_hz > 110000000ULL) {
				continue;
			}

			out->pll_en = true;
			out->p = p;
			out->j = j;
			out->d = d;
			return 0;
		}
	}

	LOG_ERR("No clock config for MCLK=%u Fs=%u", mclk, fs);
	return -EINVAL;
}

static int aic26_set_pll(const struct device *dev,
			 const struct aic26_pll_cfg *pll)
{
	uint16_t pll1;
	uint16_t pll2;
	uint8_t p_enc;
	uint8_t q_enc;
	int ret;

	if (pll->pll_en) {
		if (pll->p == 8) {
			p_enc = 0;
		} else {
			p_enc = pll->p;
		}

		pll1 = AIC26_PLLSEL |
		       FIELD_PREP(AIC26_PVAL, p_enc) |
		       FIELD_PREP(AIC26_JVAL, pll->j);
		pll2 = FIELD_PREP(AIC26_DVAL, pll->d);
	} else {
		if (pll->q == 16) {
			q_enc = 0;
		} else if (pll->q == 17) {
			q_enc = 1;
		} else {
			q_enc = pll->q;
		}

		pll1 = FIELD_PREP(AIC26_QVAL, q_enc);
		pll2 = 0;
	}

	ret = aic26_reg_write(dev, AIC26_PAGE2, AIC26_P2_PLL2, pll2);
	if (ret < 0) {
		return ret;
	}

	ret = aic26_reg_write(dev, AIC26_PAGE2, AIC26_P2_PLL1, pll1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int aic26_adc_setup(const struct device *dev)
{
	uint16_t reg;
	int ret;

	reg = FIELD_PREP(AIC26_AGCTG, AIC26_AGCTG_M12) |
	      FIELD_PREP(AIC26_AGCTC, AIC26_AGCTC_8_100) |
	      AIC26_AGCEN;

	ret = aic26_reg_write(dev, AIC26_PAGE2, AIC26_P2_ADC_GAIN, reg);
	if (ret < 0) {
		return ret;
	}

	ret = aic26_reg_update(dev, AIC26_PAGE2, AIC26_P2_AUDIO_CTL3,
			       AIC26_AGCNL, FIELD_PREP(AIC26_AGCNL, AIC26_AGCNL_M60));
	if (ret < 0) {
		return ret;
	}

	return aic26_reg_update(dev, AIC26_PAGE2, AIC26_P2_POWER_CTL,
				AIC26_PWDNC | AIC26_ADPWDN, 0);
}

static int aic26_configure(const struct device *dev,
			   struct audio_codec_cfg *cfg)
{
	const struct tlv320aic26_config *dev_cfg = dev->config;
	struct tlv320aic26_data *data = dev->data;
	struct aic26_pll_cfg pll;
	bool need_dac;
	bool need_adc;
	bool codec_master;
	uint16_t reg;
	uint8_t wlen;
	uint8_t datfm;
	int ret;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		return -EINVAL;
	}

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
		need_dac = true;
		need_adc = false;
		break;
	case AUDIO_ROUTE_CAPTURE:
		need_dac = false;
		need_adc = true;
		break;
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		need_dac = true;
		need_adc = true;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->dai_cfg.i2s.word_size) {
	case 16:
		wlen = AIC26_WLEN_16;
		break;
	case 20:
		wlen = AIC26_WLEN_20;
		break;
	case 24:
		wlen = AIC26_WLEN_24;
		break;
	case 32:
		wlen = AIC26_WLEN_32;
		break;
	default:
		return -EINVAL;
	}

	switch (cfg->dai_cfg.i2s.format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		datfm = AIC26_DATFM_I2S;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		datfm = AIC26_DATFM_LJ;
		break;
	case I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED:
		datfm = AIC26_DATFM_RJ;
		break;
	case I2S_FMT_DATA_FORMAT_PCM_SHORT:
	case I2S_FMT_DATA_FORMAT_PCM_LONG:
		datfm = AIC26_DATFM_DSP;
		break;
	default:
		return -EINVAL;
	}

	if (!!(cfg->dai_cfg.i2s.options & I2S_OPT_BIT_CLK_TARGET) !=
	    !!(cfg->dai_cfg.i2s.options & I2S_OPT_FRAME_CLK_TARGET)) {
		return -EINVAL;
	}

	codec_master = !(cfg->dai_cfg.i2s.options & I2S_OPT_BIT_CLK_TARGET);

	ret = aic26_calc_pll(dev_cfg->mclk_freq,
			     cfg->dai_cfg.i2s.frame_clk_freq, &pll);
	if (ret < 0) {
		return ret;
	}

	data->configured = false;

	ret = aic26_reg_write(dev, AIC26_PAGE1, AIC26_P1_RESET,
			      AIC26_RESET_VALUE);
	if (ret < 0) {
		return ret;
	}

	ret = aic26_reg_write(dev, AIC26_PAGE1, AIC26_P1_REFERENCE,
			      AIC26_VREFM | AIC26_IREFV);
	if (ret < 0) {
		return ret;
	}

	ret = aic26_set_pll(dev, &pll);
	if (ret < 0) {
		return ret;
	}

	reg = FIELD_PREP(AIC26_WLEN, wlen) |
	      FIELD_PREP(AIC26_DATFM, datfm);

	if (need_dac) {
		reg |= FIELD_PREP(AIC26_DACFS, pll.fs_div);
	}

	if (need_adc) {
		reg |= FIELD_PREP(AIC26_ADCHPF, AIC26_HPF_0045FS) |
		       FIELD_PREP(AIC26_ADCIN, AIC26_ADCIN_MIC) |
		       FIELD_PREP(AIC26_ADCFS, pll.fs_div);
	}

	ret = aic26_reg_write(dev, AIC26_PAGE2, AIC26_P2_AUDIO_CTL1, reg);
	if (ret < 0) {
		return ret;
	}

	reg = 0;

	if (pll.fsref_44k1) {
		reg |= AIC26_REFFS;
	}

	if (codec_master) {
		reg |= AIC26_SLVMS;
	}

	ret = aic26_reg_write(dev, AIC26_PAGE2, AIC26_P2_AUDIO_CTL3, reg);
	if (ret < 0) {
		return ret;
	}

	if (need_adc) {
		ret = aic26_adc_setup(dev);
		if (ret < 0) {
			return ret;
		}
	}

	data->configured = true;
	data->route = cfg->dai_route;

	LOG_INF("Configured: Fs=%u ws=%u %s",
		cfg->dai_cfg.i2s.frame_clk_freq,
		cfg->dai_cfg.i2s.word_size,
		codec_master ? "controller" : "target");

	return 0;
}

static void aic26_start_output(const struct device *dev)
{
	struct tlv320aic26_data *data = dev->data;
	int ret;

	if (!data->configured) {
		return;
	}

	if (data->route == AUDIO_ROUTE_CAPTURE) {
		return;
	}

	ret = aic26_reg_update(dev, AIC26_PAGE2, AIC26_P2_POWER_CTL,
			       AIC26_PWDNC | AIC26_DAPWDN |
			       AIC26_DAODRC | AIC26_ADPWDN,
			       AIC26_DAODRC | AIC26_ADPWDN);
	if (ret < 0) {
		return;
	}

	aic26_reg_update(dev, AIC26_PAGE2, AIC26_P2_DAC_GAIN,
			 AIC26_DALMU | AIC26_DARMU, 0);
}

static void aic26_stop_output(const struct device *dev)
{
	struct tlv320aic26_data *data = dev->data;

	if (!data->configured) {
		return;
	}

	aic26_reg_update(dev, AIC26_PAGE2, AIC26_P2_DAC_GAIN,
			 AIC26_DALMU | AIC26_DARMU,
			 AIC26_DALMU | AIC26_DARMU);

	aic26_reg_update(dev, AIC26_PAGE2, AIC26_P2_POWER_CTL,
			 AIC26_DAPWDN, AIC26_DAPWDN);
}

static int aic26_set_output_volume(const struct device *dev,
				   audio_channel_t channel,
				   int vol)
{
	uint16_t reg;
	uint8_t atten;
	int ret;

	atten = MIN((uint8_t)(-CLAMP(vol, -63, 0) * 2), 0x7F);

	ret = aic26_reg_read(dev, AIC26_PAGE2, AIC26_P2_DAC_GAIN, &reg);
	if (ret < 0) {
		return ret;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		reg = (reg & ~AIC26_DALVL) |
		      FIELD_PREP(AIC26_DALVL, atten);
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		reg = (reg & ~AIC26_DARVL) |
		      FIELD_PREP(AIC26_DARVL, atten);
		break;
	case AUDIO_CHANNEL_ALL:
		reg = (reg & ~(AIC26_DALVL | AIC26_DARVL)) |
		      FIELD_PREP(AIC26_DALVL, atten) |
		      FIELD_PREP(AIC26_DARVL, atten);
		break;
	default:
		return -EINVAL;
	}

	return aic26_reg_write(dev, AIC26_PAGE2, AIC26_P2_DAC_GAIN, reg);
}

static int aic26_set_output_mute(const struct device *dev,
				 audio_channel_t channel,
				 bool mute)
{
	uint16_t reg;
	int ret;

	ret = aic26_reg_read(dev, AIC26_PAGE2, AIC26_P2_DAC_GAIN, &reg);
	if (ret < 0) {
		return ret;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		if (mute) {
			reg |= AIC26_DALMU;
		} else {
			reg &= ~AIC26_DALMU;
		}
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		if (mute) {
			reg |= AIC26_DARMU;
		} else {
			reg &= ~AIC26_DARMU;
		}
		break;
	case AUDIO_CHANNEL_ALL:
		if (mute) {
			reg |= AIC26_DALMU | AIC26_DARMU;
		} else {
			reg &= ~(AIC26_DALMU | AIC26_DARMU);
		}
		break;
	default:
		return -EINVAL;
	}

	return aic26_reg_write(dev, AIC26_PAGE2, AIC26_P2_DAC_GAIN, reg);
}

static int aic26_set_property(const struct device *dev,
			      audio_property_t property,
			      audio_channel_t channel,
			      audio_property_value_t val)
{
	uint8_t gain;

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return aic26_set_output_volume(dev, channel, val.vol);

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return aic26_set_output_mute(dev, channel, val.mute);

	case AUDIO_PROPERTY_INPUT_VOLUME:
		gain = MIN((uint8_t)(CLAMP(val.vol, 0, 59) * 2), 0x77);

		return aic26_reg_update(dev, AIC26_PAGE2,
					AIC26_P2_ADC_GAIN,
					AIC26_ADMUT | AIC26_ADPGA,
					FIELD_PREP(AIC26_ADPGA, gain));

	case AUDIO_PROPERTY_INPUT_MUTE:
		return aic26_reg_update(dev, AIC26_PAGE2,
					AIC26_P2_ADC_GAIN,
					AIC26_ADMUT,
					val.mute ? AIC26_ADMUT : 0);

	default:
		return -ENOTSUP;
	}
}

static int aic26_apply_properties(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int aic26_init(const struct device *dev)
{
	const struct tlv320aic26_config *cfg = dev->config;
	struct tlv320aic26_data *data = dev->data;
	uint16_t reg;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	ret = aic26_reg_write(dev, AIC26_PAGE1, AIC26_P1_RESET,
			      AIC26_RESET_VALUE);
	if (ret < 0) {
		return ret;
	}

	ret = aic26_reg_read(dev, AIC26_PAGE2, AIC26_P2_POWER_CTL, &reg);
	if (ret < 0) {
		return ret;
	}

	if (reg == 0x0000 || reg == 0xFFFF) {
		LOG_ERR("No codec response (reg=0x%04x)", reg);
		return -EIO;
	}

	ret = aic26_reg_write(dev, AIC26_PAGE1, AIC26_P1_REFERENCE,
			      AIC26_VREFM | AIC26_IREFV);
	if (ret < 0) {
		return ret;
	}

	data->configured = false;
	LOG_INF("TLV320AIC26 ready (MCLK=%u Hz)", cfg->mclk_freq);
	return 0;
}

static const struct audio_codec_api aic26_api = {
	.configure        = aic26_configure,
	.start_output     = aic26_start_output,
	.stop_output      = aic26_stop_output,
	.set_property     = aic26_set_property,
	.apply_properties = aic26_apply_properties,
};

#define TLV320AIC26_INIT(n)						\
	static struct tlv320aic26_data aic26_data_##n;			\
									\
	static const struct tlv320aic26_config aic26_cfg_##n = {	\
		.spi = SPI_DT_SPEC_INST_GET(n,				\
			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |		\
			SPI_MODE_CPHA | SPI_WORD_SET(8)),		\
		.mclk_freq = DT_INST_PROP(n, mclk_frequency),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, aic26_init, NULL,			\
		&aic26_data_##n, &aic26_cfg_##n,			\
		POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY,		\
		&aic26_api);

DT_INST_FOREACH_STATUS_OKAY(TLV320AIC26_INIT)
