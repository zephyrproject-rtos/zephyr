/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tlv320dac

#include <errno.h>

#include <zephyr/sys/util.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/audio/codec.h>
#include "tlv320dac310x.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlv320dac310x);

#define CODEC_OUTPUT_VOLUME_MAX		0
#define CODEC_OUTPUT_VOLUME_MIN		(-78 * 2)

struct codec_driver_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
};

struct codec_driver_data {
	struct reg_addr	reg_addr_cache;
};

static struct codec_driver_config codec_device_config = {
	.bus		= I2C_DT_SPEC_INST_GET(0),
	.reset_gpio	= GPIO_DT_SPEC_INST_GET(0, reset_gpios),
};

static struct codec_driver_data codec_device_data;

static void codec_write_reg(const struct device *dev, struct reg_addr reg,
			    uint8_t val);
static void codec_read_reg(const struct device *dev, struct reg_addr reg,
			   uint8_t *val);
static void codec_soft_reset(const struct device *dev);
static int codec_configure_dai(const struct device *dev, audio_dai_cfg_t *cfg);
static int codec_configure_clocks(const struct device *dev,
				  struct audio_codec_cfg *cfg);
static int codec_configure_filters(const struct device *dev,
				   audio_dai_cfg_t *cfg);
static enum osr_multiple codec_get_osr_multiple(audio_dai_cfg_t *cfg);
static void codec_configure_output(const struct device *dev);
static int codec_set_output_volume(const struct device *dev, int vol);

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
static void codec_read_all_regs(const struct device *dev);
#define CODEC_DUMP_REGS(dev)	codec_read_all_regs((dev))
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

	if (!device_is_ready(dev_cfg->reset_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	return 0;
}

static int codec_configure(const struct device *dev,
			   struct audio_codec_cfg *cfg)
{
	const struct codec_driver_config *const dev_cfg = dev->config;
	int ret;

	if (cfg->dai_type != AUDIO_DAI_TYPE_I2S) {
		LOG_ERR("dai_type must be AUDIO_DAI_TYPE_I2S");
		return -EINVAL;
	}

	/* Configure reset GPIO, and set the line to inactive, which will also
	 * de-assert the reset line and thus enable the codec.
	 */
	gpio_pin_configure_dt(&dev_cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);

	codec_soft_reset(dev);

	ret = codec_configure_clocks(dev, cfg);
	if (ret == 0) {
		ret = codec_configure_dai(dev, &cfg->dai_cfg);
	}
	if (ret == 0) {
		ret = codec_configure_filters(dev, &cfg->dai_cfg);
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

static void codec_mute_output(const struct device *dev)
{
	/* mute DAC channels */
	codec_write_reg(dev, VOL_CTRL_ADDR, VOL_CTRL_MUTE_DEFAULT);
}

static void codec_unmute_output(const struct device *dev)
{
	/* unmute DAC channels */
	codec_write_reg(dev, VOL_CTRL_ADDR, VOL_CTRL_UNMUTE_DEFAULT);
}

static int codec_set_property(const struct device *dev,
			      audio_property_t property,
			      audio_channel_t channel,
			      audio_property_value_t val)
{
	/* individual channel control not currently supported */
	if (channel != AUDIO_CHANNEL_ALL) {
		LOG_ERR("channel %u invalid. must be AUDIO_CHANNEL_ALL",
			channel);
		return -EINVAL;
	}

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return codec_set_output_volume(dev, val.vol);

	case AUDIO_PROPERTY_OUTPUT_MUTE:
		if (val.mute) {
			codec_mute_output(dev);
		} else {
			codec_unmute_output(dev);
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
	struct codec_driver_data *const dev_data = dev->data;
	const struct codec_driver_config *const dev_cfg = dev->config;

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
	struct codec_driver_data *const dev_data = dev->data;
	const struct codec_driver_config *const dev_cfg = dev->config;

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
		LOG_ERR("Unsupported PCM sample bit width %u",
				cfg->i2s.word_size);
		return -EINVAL;
	}

	codec_write_reg(dev, IF_CTRL1_ADDR, val);
	return 0;
}

static int codec_configure_clocks(const struct device *dev,
				  struct audio_codec_cfg *cfg)
{
	int dac_clk, mod_clk;
	struct i2s_config *i2s;
	int osr, osr_min, osr_max;
	enum osr_multiple osr_multiple;
	int mdac, ndac, bclk_div, mclk_div;

	i2s = &cfg->dai_cfg.i2s;
	LOG_DBG("MCLK %u Hz PCM Rate: %u Hz", cfg->mclk_freq,
			i2s->frame_clk_freq);

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
	osr_min = (DAC_MOD_CLK_FREQ_MIN + i2s->frame_clk_freq - 1) /
		i2s->frame_clk_freq;
	osr_max = DAC_MOD_CLK_FREQ_MAX / i2s->frame_clk_freq;

	/* round mix and max values to the required multiple */
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

	LOG_DBG("Processing freq: %u Hz Modulator freq: %u Hz",
			dac_clk, mod_clk);
	LOG_DBG("NDAC: %u MDAC: %u OSR: %u", ndac, mdac, osr);

	if (i2s->options & I2S_OPT_BIT_CLK_MASTER) {
		bclk_div = osr * mdac / (i2s->word_size * 2U); /* stereo */
		if ((bclk_div * i2s->word_size * 2) != (osr * mdac)) {
			LOG_ERR("Unable to generate BCLK %u from MCLK %u",
				i2s->frame_clk_freq * i2s->word_size * 2U,
				cfg->mclk_freq);
			return -EINVAL;
		}
		LOG_DBG("I2S Master BCLKDIV: %u", bclk_div);
		codec_write_reg(dev, BCLK_DIV_ADDR,
				BCLK_DIV_POWER_UP | BCLK_DIV(bclk_div));
	}

	/* set NDAC, then MDAC, followed by OSR */
	codec_write_reg(dev, NDAC_DIV_ADDR,
			(uint8_t)(NDAC_DIV(ndac) | NDAC_POWER_UP_MASK));
	codec_write_reg(dev, MDAC_DIV_ADDR,
			(uint8_t)(MDAC_DIV(mdac) | MDAC_POWER_UP_MASK));
	codec_write_reg(dev, OSR_MSB_ADDR, (uint8_t)((osr >> 8) & OSR_MSB_MASK));
	codec_write_reg(dev, OSR_LSB_ADDR, (uint8_t)(osr & OSR_LSB_MASK));

	if (i2s->options & I2S_OPT_BIT_CLK_MASTER) {
		codec_write_reg(dev, BCLK_DIV_ADDR,
				BCLK_DIV(bclk_div) | BCLK_DIV_POWER_UP);
	}

	/* calculate MCLK divider to get ~1MHz */
	mclk_div = DIV_ROUND_UP(cfg->mclk_freq, 1000000);
	/* setup timer clock to be MCLK divided */
	codec_write_reg(dev, TIMER_MCLK_DIV_ADDR,
			TIMER_MCLK_DIV_EN_EXT | TIMER_MCLK_DIV_VAL(mclk_div));
	LOG_DBG("Timer MCLK Divider: %u", mclk_div);

	return 0;
}

static int codec_configure_filters(const struct device *dev,
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
	return 0;
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

	/* route DAC output to Headphone */
	val = OUTPUT_ROUTING_HPL | OUTPUT_ROUTING_HPR;
	codec_write_reg(dev, OUTPUT_ROUTING_ADDR, val);

	/* enable volume control on Headphone out */
	codec_write_reg(dev, HPL_ANA_VOL_CTRL_ADDR,
			HPX_ANA_VOL(HPX_ANA_VOL_DEFAULT));
	codec_write_reg(dev, HPR_ANA_VOL_CTRL_ADDR,
			HPX_ANA_VOL(HPX_ANA_VOL_DEFAULT));

	/* set headphone outputs as line-out */
	codec_write_reg(dev, HEADPHONE_DRV_CTRL_ADDR, HEADPHONE_DRV_LINEOUT);

	/* unmute headphone drivers */
	codec_write_reg(dev, HPL_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE);
	codec_write_reg(dev, HPR_DRV_GAIN_CTRL_ADDR, HPX_DRV_UNMUTE);

	/* power up headphone drivers */
	codec_read_reg(dev, HEADPHONE_DRV_ADDR, &val);
	val |= HEADPHONE_DRV_POWERUP | HEADPHONE_DRV_RESERVED;
	codec_write_reg(dev, HEADPHONE_DRV_ADDR, val);
}

static int codec_set_output_volume(const struct device *dev, int vol)
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

	codec_write_reg(dev, HPL_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(vol_val));
	codec_write_reg(dev, HPR_ANA_VOL_CTRL_ADDR, HPX_ANA_VOL(vol_val));
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

static const struct audio_codec_api codec_driver_api = {
	.configure		= codec_configure,
	.start_output		= codec_start_output,
	.stop_output		= codec_stop_output,
	.set_property		= codec_set_property,
	.apply_properties	= codec_apply_properties,
};

DEVICE_DT_INST_DEFINE(0, codec_initialize, NULL, &codec_device_data,
		&codec_device_config, POST_KERNEL,
		CONFIG_AUDIO_CODEC_INIT_PRIORITY, &codec_driver_api);
