/*
 * Copyright (c) 2025 PHYTEC America LLC
 * Author: Florijan Plohl <florijan.plohl@norik.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlv320aic3110

#include <errno.h>

#include <zephyr/sys/util.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/audio/codec.h>
#include <zephyr/drivers/clock_control.h>
#include "tlv320aic3110.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlv320aic3110);

#define CODEC_OUTPUT_VOLUME_MAX 0
#define CODEC_OUTPUT_VOLUME_MIN (-78 * 2)

struct codec_driver_config {
	struct i2c_dt_spec bus;
	int clock_source;
	const struct device *mclk_dev;
	clock_control_subsys_t mclk_name;
};

struct codec_driver_data {
	struct reg_addr reg_addr_cache;
};

static void codec_write_reg(const struct device *dev, struct reg_addr reg, uint8_t val);
static void codec_read_reg(const struct device *dev, struct reg_addr reg, uint8_t *val);
static void codec_update_reg(const struct device *dev, struct reg_addr reg, uint8_t mask,
			     uint8_t val);
static void codec_soft_reset(const struct device *dev);
static int codec_configure_dai(const struct device *dev, audio_dai_cfg_t *cfg);
static int codec_configure_clocks(const struct device *dev, struct audio_codec_cfg *cfg);
static int codec_configure_filters(const struct device *dev, audio_dai_cfg_t *cfg);
static void codec_configure_output(const struct device *dev);
static void codec_configure_input(const struct device *dev);
static int codec_set_output_volume(const struct device *dev, audio_channel_t channel, int vol);

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
static void codec_read_all_regs(const struct device *dev);
#define CODEC_DUMP_REGS(dev) codec_read_all_regs((dev))
#else
#define CODEC_DUMP_REGS(dev)
#endif

static int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	int ret;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("dai_type must be AUDIO_DAI_TYPE_I2S");
		return -EINVAL;
	}

	codec_soft_reset(dev);

	ret = codec_configure_clocks(dev, cfg);
	if (ret) {
		LOG_ERR("Failed to configure clocks: %d", ret);
		return ret;
	}

	ret = codec_configure_dai(dev, &cfg->dai_cfg);
	if (ret) {
		LOG_ERR("Failed to configure DAI: %d", ret);
		return ret;
	}

	ret = codec_configure_filters(dev, &cfg->dai_cfg);
	if (ret) {
		LOG_ERR("Failed to configure filters: %d", ret);
		return ret;
	}

	codec_configure_input(dev);
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

static void codec_mute_output(const struct device *dev, audio_channel_t channel, bool mute)
{
	/* mute channel */
	switch (channel) {
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		/**/
		codec_update_reg(dev, HPL_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE,
				 (mute << 2) | HPX_DRV_RESERVED);
		break;

	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		codec_update_reg(dev, HPR_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE,
				 (mute << 2) | HPX_DRV_RESERVED);
		break;

	case AUDIO_CHANNEL_FRONT_LEFT:
		codec_update_reg(dev, SPL_DRV_GAIN_CTRL_ADDR, SPX_DRV_UNMUTE, mute << 2);
		break;

	case AUDIO_CHANNEL_FRONT_RIGHT:
		codec_update_reg(dev, SPR_DRV_GAIN_CTRL_ADDR, SPX_DRV_UNMUTE, mute << 2);
		break;

	case AUDIO_CHANNEL_ALL:
		codec_update_reg(dev, HPL_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE,
				 (mute << 2) | HPX_DRV_RESERVED);
		codec_update_reg(dev, HPR_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE,
				 (mute << 2) | HPX_DRV_RESERVED);
		codec_update_reg(dev, SPL_DRV_GAIN_CTRL_ADDR, SPX_DRV_UNMUTE, mute << 2);
		codec_update_reg(dev, SPR_DRV_GAIN_CTRL_ADDR, SPX_DRV_UNMUTE, mute << 2);
		break;

	default:
		LOG_ERR("channel %u invalid.", channel);
		break;
	}
}

static int codec_set_property(const struct device *dev, audio_property_t property,
			      audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return codec_set_output_volume(dev, channel, val.vol);

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		codec_mute_output(dev, channel, !val.mute);
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

static void codec_write_reg(const struct device *dev, struct reg_addr reg, uint8_t val)
{
	struct codec_driver_data *const dev_data = dev->data;
	const struct codec_driver_config *const dev_cfg = dev->config;

	/* set page if different */
	if (dev_data->reg_addr_cache.page != reg.page) {
		i2c_reg_write_byte_dt(&dev_cfg->bus, 0, reg.page);
		dev_data->reg_addr_cache.page = reg.page;
	}

	i2c_reg_write_byte_dt(&dev_cfg->bus, reg.reg_addr, val);
	LOG_DBG("WR PG:%u REG:%02u VAL:0x%02x", reg.page, reg.reg_addr, val);
}

static void codec_read_reg(const struct device *dev, struct reg_addr reg, uint8_t *val)
{
	struct codec_driver_data *const dev_data = dev->data;
	const struct codec_driver_config *const dev_cfg = dev->config;

	/* set page if different */
	if (dev_data->reg_addr_cache.page != reg.page) {
		i2c_reg_write_byte_dt(&dev_cfg->bus, 0, reg.page);
		dev_data->reg_addr_cache.page = reg.page;
	}

	i2c_reg_read_byte_dt(&dev_cfg->bus, reg.reg_addr, val);
	LOG_DBG("RD PG:%u REG:%02u VAL:0x%02x", reg.page, reg.reg_addr, *val);
}

static void codec_update_reg(const struct device *dev, struct reg_addr reg, uint8_t mask,
			     uint8_t val)
{
	uint8_t reg_val;

	codec_read_reg(dev, reg, &reg_val);
	reg_val = (reg_val & ~mask) | (val & mask);
	codec_write_reg(dev, reg, reg_val);
}

static void codec_soft_reset(const struct device *dev)
{
	/* soft reset the AIC */
	codec_write_reg(dev, SOFT_RESET_ADDR, SOFT_RESET_ASSERT);
}

static int codec_configure_dai(const struct device *dev, audio_dai_cfg_t *cfg)
{
	uint8_t val;

	/* configure I2S interface */
	val = IF_CTRL_IFTYPE(IF_CTRL_IFTYPE_I2S);
	if (cfg->i2s.options & I2S_OPT_BIT_CLK_MASTER) {
		val |= IF_CTRL_BCLK_OUT;
	}

	if (cfg->i2s.options & I2S_OPT_FRAME_CLK_MASTER) {
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
		LOG_ERR("Unsupported PCM sample bit width %u", cfg->i2s.word_size);
		return -EINVAL;
	}

	codec_write_reg(dev, IF_CTRL1_ADDR, val);
	return 0;
}

static int codec_configure_clocks(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct i2s_config *i2s;
	const struct codec_driver_config *const dev_cfg = dev->config;
	uint32_t rate;
	int madc, nadc, aosr, mdac, ndac, dosr, bclk_div, mclk_div;
	int p, r, j, d;
	int i, ret;

	i2s = &cfg->dai_cfg.i2s;

	/* get MCLK rate */
	if (dev_cfg->clock_source == 0) {
		ret = clock_control_get_rate(dev_cfg->mclk_dev, dev_cfg->mclk_name,
					     &cfg->mclk_freq);
		if (ret < 0) {
			LOG_ERR("MCLK clock source freq acquire fail: %d", ret);
		}
	}

	LOG_DBG("MCLK %u Hz Sampling Rate: %u Hz", cfg->mclk_freq, i2s->frame_clk_freq);

	/* use pll div table to get all values */
	rate = i2s->frame_clk_freq;
	for (i = 0; i < ARRAY_SIZE(pll_div_table); i++) {
		if (rate == pll_div_table[i].rate && cfg->mclk_freq == pll_div_table[i].mclk) {
			break;
		}
	}

	if (i == ARRAY_SIZE(pll_div_table)) {
		LOG_ERR("Unable to find PLL dividers for MCLK %u Hz PCM Rate: %u Hz",
			cfg->mclk_freq, i2s->frame_clk_freq);
		return -EINVAL;
	}

	p = pll_div_table[i].pll_p;
	r = 1;
	j = pll_div_table[i].pll_j;
	d = pll_div_table[i].pll_d;

	/* set the dividers */
	codec_write_reg(dev, PLL_P_R_ADDR, (1 << 7) | PLL_P(p) | PLL_R(r));
	codec_write_reg(dev, PLL_J_ADDR, j);
	codec_write_reg(dev, PLL_D_MSB_ADDR, (uint8_t)(d >> 8));
	codec_write_reg(dev, PLL_D_LSB_ADDR, (uint8_t)(d & 0xFF));

	madc = pll_div_table[i].madc;
	nadc = pll_div_table[i].nadc;
	aosr = pll_div_table[i].aosr;

	ndac = pll_div_table[i].ndac;
	mdac = pll_div_table[i].mdac;
	dosr = pll_div_table[i].dosr;

	LOG_DBG("PLLP: %u PLLJ: %u PLLD: %u", pll_div_table[i].pll_p, pll_div_table[i].pll_j,
		pll_div_table[i].pll_d);
	LOG_DBG("MDAC: %u NDAC: %u DOSR: %u", mdac, ndac, dosr);
	LOG_DBG("MADC: %u NADC: %u AOSR: %u", madc, nadc, aosr);

	if (i2s->options & I2S_OPT_BIT_CLK_MASTER) {
		bclk_div = dosr * mdac / (i2s->word_size * 2U); /* stereo */
		if ((bclk_div * i2s->word_size * 2) != (dosr * mdac)) {
			LOG_ERR("Unable to generate BCLK %u from MCLK %u",
				i2s->frame_clk_freq * i2s->word_size * 2U, cfg->mclk_freq);
			return -EINVAL;
		}
		LOG_DBG("I2S Master BCLKDIV: %u", bclk_div);
		codec_write_reg(dev, BCLK_DIV_ADDR, BCLK_DIV_POWER_UP | BCLK_DIV(bclk_div));
	}

	/* Set clock gen mux and turn on PLL */
	codec_write_reg(dev, CLOCK_GEN_MUX_ADDR, CLOCK_GEN_MUX_DEFAULT);
	codec_update_reg(dev, PLL_P_R_ADDR, PLL_POWER_UP, PLL_POWER_UP);

	/* set NDAC, then MDAC, followed by OSR */
	codec_write_reg(dev, NDAC_DIV_ADDR, (uint8_t)(NDAC_DIV(ndac) | NDAC_POWER_UP_MASK));
	codec_write_reg(dev, MDAC_DIV_ADDR, (uint8_t)(MDAC_DIV(mdac) | MDAC_POWER_UP_MASK));
	codec_write_reg(dev, OSR_MSB_ADDR, (uint8_t)((dosr >> 8) & OSR_MSB_MASK));
	codec_write_reg(dev, OSR_LSB_ADDR, (uint8_t)(dosr & OSR_LSB_MASK));

	/* set NADC, MADC, OSR */
	codec_write_reg(dev, NADC_DIV_ADDR, (uint8_t)(NADC_DIV(nadc) | NADC_POWER_UP_MASK));
	codec_write_reg(dev, MADC_DIV_ADDR, (uint8_t)(MADC_DIV(madc) | MADC_POWER_UP_MASK));
	codec_write_reg(dev, AOSR_ADDR, (uint8_t)aosr);

	if (i2s->options & I2S_OPT_BIT_CLK_MASTER) {
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

static int codec_configure_filters(const struct device *dev, audio_dai_cfg_t *cfg)
{
	enum dac_proc_block dac_proc_blk;
	enum adc_proc_block adc_proc_blk;

	/* determine decimation filter type */
	if (cfg->i2s.frame_clk_freq >= AUDIO_PCM_RATE_192K) {
		dac_proc_blk = PRB_P17_DECIMATION_C;
		adc_proc_blk = PRB_R16_DECIMATION_C;
		LOG_INF("PCM Rate: %u Filter C PRB P17/R16 selected", cfg->i2s.frame_clk_freq);
	} else if (cfg->i2s.frame_clk_freq >= AUDIO_PCM_RATE_96K) {
		dac_proc_blk = PRB_P7_DECIMATION_B;
		adc_proc_blk = PRB_R10_DECIMATION_B;
		LOG_INF("PCM Rate: %u Filter B PRB P7/R10 selected", cfg->i2s.frame_clk_freq);
	} else {
		dac_proc_blk = PRB_P1_DECIMATION_A;
		adc_proc_blk = PRB_R4_DECIMATION_A;
		LOG_INF("PCM Rate: %u Filter A PRB P1/R4 selected", cfg->i2s.frame_clk_freq);
	}

	codec_write_reg(dev, DAC_PROC_BLK_SEL_ADDR, dac_proc_blk);
	codec_write_reg(dev, ADC_PROC_BLK_SEL_ADDR, adc_proc_blk);
	return 0;
}

static void codec_configure_output(const struct device *dev)
{
	uint8_t val;

	/*
	 * set common mode voltage to 1.65V (half of AVDD)
	 * AVDD is typically 3.3V
	 */
	codec_read_reg(dev, HEADPHONE_DRV_ADDR, &val);
	val &= ~HEADPHONE_DRV_CM_MASK;
	val |= HEADPHONE_DRV_CM(CM_VOLTAGE_1P65) | HEADPHONE_DRV_RESERVED;
	codec_write_reg(dev, HEADPHONE_DRV_ADDR, val);

	/* enable pop removal on power down/up */
	codec_read_reg(dev, HP_OUT_POP_RM_ADDR, &val);
	codec_write_reg(dev, HP_OUT_POP_RM_ADDR, val | HP_OUT_POP_RM_ENABLE);

	/* route DAC output to mixer */
	val = OUTPUT_ROUTING_MIXER;
	codec_write_reg(dev, OUTPUT_ROUTING_ADDR, val);

	/* enable volume control on Headphone out and Speaker out */
	codec_write_reg(dev, HPL_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(HPX_ANA_VOL_DEFAULT));
	codec_write_reg(dev, HPR_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(HPX_ANA_VOL_DEFAULT));
	codec_write_reg(dev, SPL_ANA_VOL_CTRL_ADDR, SPX_ANA_VOL(SPX_ANA_VOL_DEFAULT));
	codec_write_reg(dev, SPR_ANA_VOL_CTRL_ADDR, SPX_ANA_VOL(SPX_ANA_VOL_DEFAULT));

	/* unmute headphone and speaker drivers */
	codec_write_reg(dev, HPL_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE | HPX_DRV_RESERVED);
	codec_write_reg(dev, HPR_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE | HPX_DRV_RESERVED);
	codec_write_reg(dev, SPL_DRV_GAIN_CTRL_ADDR, SPX_DRV_UNMUTE);
	codec_write_reg(dev, SPR_DRV_GAIN_CTRL_ADDR, SPX_DRV_UNMUTE);

	/* power up headphone drivers */
	codec_read_reg(dev, HEADPHONE_DRV_ADDR, &val);
	val |= HEADPHONE_DRV_POWERUP | HEADPHONE_DRV_RESERVED;
	codec_write_reg(dev, HEADPHONE_DRV_ADDR, val);

	/* power up speaker drivers */
	codec_read_reg(dev, SPEAKER_DRV_ADDR, &val);
	val |= SPEAKER_DRV_POWERUP | SPEAKER_DRV_RESERVED;
	codec_write_reg(dev, SPEAKER_DRV_ADDR, val);
	LOG_INF("Headphone driver and Class-D amplifier powered up");
}

static void codec_configure_input(const struct device *dev)
{
	uint8_t val;

	/* power up ADC channel */
	codec_write_reg(dev, MIC_ADC_CTRL_ADDR, MIC_ADC_POWERUP);

	/* set microphone bias - THIS SHOULD BE FIXED*/
	codec_write_reg(dev, MIC_BIAS_ADDR, MICBIAS_DEFAULT);

	/* unmute microphone input */
	codec_write_reg(dev, MIC_FCTRL_ADDR, MIC_FCTRL_DEFAULT);

	/* set PGA, D7 enables PGA control, D6-D0 sets volume */
	codec_write_reg(dev, MIC_PGA_ADDR, MIC_PGA_VOL_DEFAULT);

	/* select both MIC inputs for PGA and their resistance */
	val = MIC_PGAPI_L_DEFAULT | MIC_PGAPI_R_DEFAULT;
	codec_write_reg(dev, MIC_PGAPI_ADDR, val);
	LOG_INF("Microphone bias and PGA configured");
}

static int codec_set_output_volume(const struct device *dev, audio_channel_t channel, int vol)
{
	struct reg_addr reg;
	uint8_t vol_val;
	int vol_index;
	uint8_t vol_array[] = {107, 108, 110, 113, 116, 120, 125, 128, 132, 138, 144};

	if ((vol > CODEC_OUTPUT_VOLUME_MAX) || (vol < CODEC_OUTPUT_VOLUME_MIN)) {
		LOG_ERR("Invalid volume %d.%d dB", vol >> 1, ((uint32_t)vol & 1) ? 5 : 0);
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
	LOG_INF("Writing value to %u channel: %u", channel, vol_val);

	switch (channel) {
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		reg = HPL_ANA_VOL_CTRL_ADDR;
		break;

	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		reg = HPR_ANA_VOL_CTRL_ADDR;
		break;

	case AUDIO_CHANNEL_FRONT_LEFT:
		reg = SPL_ANA_VOL_CTRL_ADDR;
		break;

	case AUDIO_CHANNEL_FRONT_RIGHT:
		reg = SPR_ANA_VOL_CTRL_ADDR;
		break;

	case AUDIO_CHANNEL_ALL:
		codec_write_reg(dev, HPL_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(vol_val));
		codec_write_reg(dev, HPR_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(vol_val));
		codec_write_reg(dev, SPL_ANA_VOL_CTRL_ADDR, SPX_ANA_VOL(vol_val));
		codec_write_reg(dev, SPR_ANA_VOL_CTRL_ADDR, SPX_ANA_VOL(vol_val));
		return 0;

	default:
		LOG_ERR("channel %u invalid.", channel);
		return -EINVAL;
	}

	codec_write_reg(dev, reg, HPX_ANA_VOL(vol_val));
	return 0;
}

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
static void codec_read_all_regs(const struct device *dev)
{
	uint8_t val;

	codec_read_reg(dev, SOFT_RESET_ADDR, &val);
	codec_read_reg(dev, PLL_P_R_ADDR, &val);
	codec_read_reg(dev, PLL_J_ADDR, &val);
	codec_read_reg(dev, PLL_D_MSB_ADDR, &val);
	codec_read_reg(dev, PLL_D_LSB_ADDR, &val);
	codec_read_reg(dev, NDAC_DIV_ADDR, &val);
	codec_read_reg(dev, MDAC_DIV_ADDR, &val);
	codec_read_reg(dev, OSR_MSB_ADDR, &val);
	codec_read_reg(dev, OSR_LSB_ADDR, &val);
	codec_read_reg(dev, NADC_DIV_ADDR, &val);
	codec_read_reg(dev, MADC_DIV_ADDR, &val);
	codec_read_reg(dev, AOSR_ADDR, &val);
	codec_read_reg(dev, IF_CTRL1_ADDR, &val);
	codec_read_reg(dev, BCLK_DIV_ADDR, &val);
	codec_read_reg(dev, OVF_FLAG_ADDR, &val);
	codec_read_reg(dev, DAC_PROC_BLK_SEL_ADDR, &val);
	codec_read_reg(dev, ADC_PROC_BLK_SEL_ADDR, &val);
	codec_read_reg(dev, DATA_PATH_SETUP_ADDR, &val);
	codec_read_reg(dev, VOL_CTRL_ADDR, &val);
	codec_read_reg(dev, L_DIG_VOL_CTRL_ADDR, &val);

	codec_read_reg(dev, MIC_PGA_ADDR, &val);

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

	codec_read_reg(dev, SPL_DRV_GAIN_CTRL_ADDR, &val);
	codec_read_reg(dev, SPR_DRV_GAIN_CTRL_ADDR, &val);

	codec_read_reg(dev, TIMER_MCLK_DIV_ADDR, &val);
}
#endif

static const struct audio_codec_api codec_driver_api = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
};

#define TLV320AIC3110_INIT(n)                                                                      \
	static const struct codec_driver_config codec_device_config_##n = {                        \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.clock_source = 0,                                                                 \
		.mclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, mclk)),                   \
		.mclk_name = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, name)};  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &codec_device_config_##n, POST_KERNEL,          \
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLV320AIC3110_INIT)
