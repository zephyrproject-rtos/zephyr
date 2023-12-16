/*
 * Copyright (c) 2023 NXP Semiconductor INC.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/audio/codec.h>

#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wolfson_wm8904);

#include "wm8904.h"

#define DT_DRV_COMPAT wolfson_wm8904

#define CODEC_OUTPUT_VOLUME_MAX 0
#define CODEC_OUTPUT_VOLUME_MIN (-78 * 2)

struct codec_driver_config {
	struct i2c_dt_spec i2c;
	int    clock_source;
	const struct pinctrl_dev_config *pincfg;
};

#define WM8904_CACHEREGNUM 98
struct codec_driver_data {
	uint16_t reg_cache[WM8904_CACHEREGNUM];
};

static const uint16_t wm8904_reg[WM8904_CACHEREGNUM] = {
	0x00, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x0C, 0x0E, 0x0F, 0x12,
	0x14, 0x15, 0x16, 0x18, 0x19, 0x1A, 0x1B, 0x1E, 0x1F, 0x20,
	0x21, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C,
	0x2D, 0x2E, 0x2F, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x43, 0x44,
	0x45, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x5A, 0x5E,
	0x62, 0x68, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x74, 0x75, 0x76,
	0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7E, 0x7F, 0x80, 0x81,
	0x82, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E,
	0x8F, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0xC6, 0xF7, 0xF8};


#define DEV_CFG(dev) ((const struct codec_driver_config *const)dev->config)
#define DEV_DATA(dev) ((struct codec_driver_data *)dev->data)

static void codec_write_reg(const struct device *dev, uint8_t reg,
			    uint16_t val);
static void codec_read_reg(const struct device *dev, uint8_t reg,
			   uint16_t *val);
static void codec_update_reg(const struct device *dev, uint8_t reg,
			     uint16_t mask, uint16_t val);
static void codec_soft_reset(const struct device *dev);

static void codec_configure_output(const struct device *dev);

static int codec_initialize(const struct device *dev)
{
	int error;
	const struct codec_driver_config *const dev_cfg = DEV_CFG(dev);
	struct codec_driver_data *const dev_data = DEV_DATA(dev);

	error = pinctrl_apply_state(dev_cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (error) {
		LOG_ERR("WM8904 init PIN ERROR!");
		return error;
	}

	memcpy(dev_data->reg_cache, wm8904_reg, sizeof(wm8904_reg));

	LOG_DBG("WM8904 init");
	i3c_recover_bus(dev_cfg->i2c.bus);

	return 0;
}

static int codec_protol_config(const struct device *dev,
			       audio_dai_type_t dai_type)
{
	wm8904_protocol_t protocol;

	switch (dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		protocol = kWM8904_ProtocolI2S;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		protocol = kWM8904_ProtocolLeftJustified;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		protocol = kWM8904_ProtocolRightJustified;
		break;
	case AUDIO_DAI_TYPE_PCMA:
		protocol = kWM8904_ProtocolPCMA;
		break;
	case AUDIO_DAI_TYPE_PCMB:
		protocol = kWM8904_ProtocolPCMB;
		break;
	default:
		return -EINVAL;
	}

	codec_update_reg(dev, WM8904_AUDIO_IF_1,
			 (0x0003U | (1U << 4U)),
			 (uint16_t)protocol);

	LOG_DBG("protocol is 0x%x", protocol);
	return 0;
}

static int codec_audio_fmt_config(const struct device *dev,
		audio_dai_cfg_t *cfg, uint32_t mclk)
{
	wm8904_sample_rate_t wm_sample_rate;
	uint32_t fs;
	uint16_t wmfs_ratio;
	uint16_t mclkDiv;
	uint16_t word_size = cfg->i2s.word_size;

	switch (cfg->i2s.frame_clk_freq) {
	case 8000:
		wm_sample_rate = kWM8904_SampleRate8kHz;
		break;
	case  12000:
		wm_sample_rate = kWM8904_SampleRate12kHz;
		break;
	case 16000:
		wm_sample_rate = kWM8904_SampleRate16kHz;
		break;
	case 24000:
		wm_sample_rate = kWM8904_SampleRate24kHz;
		break;
	case 32000:
		wm_sample_rate = kWM8904_SampleRate32kHz;
		break;
	case 48000:
		wm_sample_rate = kWM8904_SampleRate48kHz;
		break;
	case 11025:
		wm_sample_rate = kWM8904_SampleRate11025Hz;
		break;
	case 22050:
		wm_sample_rate = kWM8904_SampleRate22050Hz;
		break;
	case 44100:
		wm_sample_rate = kWM8904_SampleRate44100Hz;
		break;
	default:
		LOG_WRN("sample_rate invalid %d", cfg->i2s.frame_clk_freq);
		return -EINVAL;
	}

	codec_read_reg(dev, WM8904_CLK_RATES_0, &mclkDiv);
	fs = (mclk >> (mclkDiv & 0x1U)) / cfg->i2s.frame_clk_freq;

	switch (fs) {
	case 64:
		wmfs_ratio = kWM8904_FsRatio64X;
		break;
	case 128:
		wmfs_ratio = kWM8904_FsRatio128X;
		break;
	case 192:
		wmfs_ratio = kWM8904_FsRatio192X;
		break;
	case 256:
		wmfs_ratio = kWM8904_FsRatio256X;
		break;
	case 384:
		wmfs_ratio = kWM8904_FsRatio384X;
		break;
	case 512:
		wmfs_ratio = kWM8904_FsRatio512X;
		break;
	case 768:
		wmfs_ratio = kWM8904_FsRatio768X;
		break;
	case 1024:
		wmfs_ratio = kWM8904_FsRatio1024X;
		break;
	case 1408:
		wmfs_ratio = kWM8904_FsRatio1408X;
		break;
	case 1536:
		wmfs_ratio = kWM8904_FsRatio1536X;
		break;
	default:
		LOG_WRN("fs Ratio invalid %d", fs);
		return -EINVAL;
	}

	LOG_DBG("fs Ratio set to %d", wmfs_ratio);
	/* Disable SYSCLK */
	codec_write_reg(dev, WM8904_CLK_RATES_2, 0x00);

	/* Set Clock ratio and sample rate */
	codec_write_reg(dev, WM8904_CLK_RATES_1,
		((wmfs_ratio) << 10U) | (uint16_t)(wm_sample_rate));

	switch (cfg->i2s.word_size) {
	case 16:
		word_size = 0;
		break;
	case 20:
		word_size = 1;
		break;
	case 24:
		word_size = 2;
		break;
	case 32:
		word_size = 3;
		break;
	default:
		LOG_ERR("word-size not support %d force to 16 bit", cfg->i2s.word_size);
		word_size = 0;
		break;
	}
	/* Set bit resolution */
	codec_update_reg(dev, WM8904_AUDIO_IF_1,
		(0x000CU), ((uint16_t)(word_size) << 2U));
	{
		uint16_t reg_val = 0;

		codec_read_reg(dev, WM8904_AUDIO_IF_1, &reg_val);
		LOG_INF("WM8904_AUDIO_IF_1 = 0x%x", reg_val);
	}

	/* Enable SYSCLK */
	codec_write_reg(dev, WM8904_CLK_RATES_2, 0x1007);
	return 0;
}

static void wm8904_set_master_clock(const struct device *dev,
				audio_dai_cfg_t *cfg, uint32_t sysclk)
{
	uint32_t sampleRate     = cfg->i2s.frame_clk_freq;
	uint32_t bitWidth       = cfg->i2s.word_size;
	uint32_t bclk           = sampleRate * bitWidth * 2U;
	uint32_t bclkDiv        = 0U;
	uint16_t audioInterface = 0U;
	uint16_t sysclkDiv      = 0U;

	codec_read_reg(dev, WM8904_CLK_RATES_0, &sysclkDiv);
	sysclk = sysclk >> (sysclkDiv & 0x1U);
	LOG_INF("codec system clk %d", sysclk);

	if ((sysclk / bclk > 48U) || (bclk / sampleRate > 2047U) || (bclk / sampleRate < 8U)) {
		LOG_ERR("clock for wm8904 invlide");
		return;
	}

	codec_read_reg(dev, WM8904_AUDIO_IF_2, &audioInterface);

	audioInterface &= ~(uint16_t)0x1FU;
	bclkDiv = (sysclk * 10U) / bclk;
	LOG_INF("blk %d", bclk);

	switch (bclkDiv) {
	case 10:
		audioInterface |= 0U;
		break;
	case 15:
		audioInterface |= 1U;
		break;
	case 20:
		/* Avoid MISRA 16.4 violation */
		break;
	case 30:
		/* Avoid MISRA 16.4 violation */
		break;
	case 40:
		/* Avoid MISRA 16.4 violation */
		break;
	case 50:
		audioInterface |= (uint16_t)bclkDiv / 10U;
		break;
	case 55:
		audioInterface |= 6U;
		break;
	case 60:
		audioInterface |= 7U;
		break;
	case 80:
		audioInterface |= 8U;
		break;
	case 100:
		/* Avoid MISRA 16.4 violation */
		break;
	case 110:
		/* Avoid MISRA 16.4 violation */
		break;
	case 120:
		audioInterface |= (uint16_t)bclkDiv / 10U - 1U;
		break;
	case 160:
		audioInterface |= 12U;
		break;
	case 200:
		audioInterface |= 13U;
		break;
	case 220:
		audioInterface |= 14U;
		break;
	case 240:
		audioInterface |= 15U;
		break;
	case 250:
		audioInterface |= 16U;
		break;
	case 300:
		audioInterface |= 17U;
		break;
	case 320:
		audioInterface |= 18U;
		break;
	case 440:
		audioInterface |= 19U;
		break;
	case 480:
		audioInterface |= 20U;
		break;
	default:
		LOG_ERR("invalid autio interface for wm8904 %d", bclkDiv);
		return;
	}

	/* bclk divider */
	codec_write_reg(dev, WM8904_AUDIO_IF_2, audioInterface);
	/* bclk direction output */
	codec_update_reg(dev, WM8904_AUDIO_IF_1, 1U << 6U, 1U << 6U);

	codec_update_reg(dev, WM8904_GPIO_CONTROL_4, 0x8FU, 1U);
	/* LRCLK direction and divider */
	audioInterface = (uint16_t)((1UL << 11U) | (bclk / sampleRate));
	codec_update_reg(dev, WM8904_AUDIO_IF_3, 0xFFFU, audioInterface);
}

static int codec_configure(const struct device *dev,
			   struct audio_codec_cfg *cfg)
{
	uint16_t value;
	uint32_t sysclk;
	const struct codec_driver_config *const dev_cfg = DEV_CFG(dev);

	printk("CODEC WM8904 config");
	if (cfg->dai_type >= AUDIO_DAI_TYPE_INVALID) {
		LOG_ERR("dai_type not supported");
		return -EINVAL;
	}

	codec_soft_reset(dev);
	/* MCLK_INV=0, SYSCLK_SRC=0, TOCLK_RATE=0, OPCLK_ENA=1,
	 * CLK_SYS_ENA=1, CLK_DSP_ENA=1, TOCLK_ENA=1
	 */
	codec_write_reg(dev, WM8904_CLK_RATES_2, 0x000F);

	/* WSEQ_ENA=1, WSEQ_WRITE_INDEX=0_0000 */
	codec_write_reg(dev, WM8904_WRT_SEQUENCER_0, 0x0100);

	/* WSEQ_ABORT=0, WSEQ_START=1, WSEQ_START_INDEX=00_0000 */
	codec_write_reg(dev, WM8904_WRT_SEQUENCER_3, 0x0100);

	do {
		codec_read_reg(dev, WM8904_WRT_SEQUENCER_4, &value);
	} while (((value & 1U) != 0U));

	/* TOCLK_RATE_DIV16=0, TOCLK_RATE_x4=1, SR_MODE=0, MCLK_DIV=1
	 * (Required for MMCs: SGY, KRT see erratum CE000546)
	 */
	codec_write_reg(dev, WM8904_CLK_RATES_0, 0xA45F);

	/* INL_ENA=1, INR ENA=1 */
	codec_write_reg(dev, WM8904_POWER_MGMT_0, 0x0003);

	/* HPL_PGA_ENA=1, HPR_PGA_ENA=1 */
	codec_write_reg(dev, WM8904_POWER_MGMT_2, 0x0003);

	/* DACL_ENA=1, DACR_ENA=1, ADCL_ENA=1, ADCR_ENA=1 */
	codec_write_reg(dev, WM8904_POWER_MGMT_6, 0x000F);

	/* ADC_OSR128=1 */
	codec_write_reg(dev, WM8904_ANALOG_ADC_0, 0x0001);

	/* DACL_DATINV=0, DACR_DATINV=0, DAC_BOOST=00, LOOPBACK=0, AIFADCL_SRC=0,
	 * AIFADCR_SRC=1, AIFDACL_SRC=0, AIFDACR_SRC=1, ADC_COMP=0, ADC_COMPMODE=0,
	 * DAC_COMP=0, DAC_COMPMODE=0
	 */
	codec_write_reg(dev, WM8904_AUDIO_IF_0, 0x0050);

	/* DAC_MONO=0, DAC_SB_FILT-0, DAC_MUTERATE=0, DAC_UNMUTE RAMP=0,
	 * DAC_OSR128=1, DAC_MUTE=0, DEEMPH=0 (none)
	 */
	codec_write_reg(dev, WM8904_DAC_DIG_1, 0x0040);

	/* LINMUTE=0, LIN_VOL=0_0101 */
	codec_write_reg(dev, WM8904_ANALOG_LEFT_IN_0, 0x0005);

	/* RINMUTE=0, RIN VOL=0_0101 LINEOUTL RMV SHORT-1, LINEOUTL ENA_OUTP=1,
	 * LINEOUTL_ENA_DLY=1, LINEOUTL_ENA=1, LINEOUTR_RMV_SHORT-1,
	 * LINEOUTR_ENA_OUTP=1
	 */
	codec_write_reg(dev, WM8904_ANALOG_RIGHT_IN_0, 0x0005);

	/* HPOUTL_MUTE=0, HPOUT_VU=0, HPOUTLZC=0, HPOUTL_VOL=10_1101 */
	codec_write_reg(dev, WM8904_ANALOG_OUT1_LEFT, 0x00AD);

	/* HPOUTR_MUTE=0, HPOUT_VU=0, HPOUTRZC=0, HPOUTR_VOL=10_1101 */
	codec_write_reg(dev, WM8904_ANALOG_OUT1_RIGHT, 0x00AD);

	/* Enable DC servos for headphone out */
	codec_write_reg(dev, WM8904_DC_SERVO_0, 0x0003);

	/* HPL_RMV_SHORT=1, HPL_ENA_OUTP=1, HPL_ENA_DLY=1, HPL_ENA=1,
	 * HPR_RMV_SHORT=1, HPR_ENA_OUTP=1, HPR_ENA_DLY=1, HPR_ENA=1
	 */
	codec_write_reg(dev, WM8904_ANALOG_HP_0, 0x00FF);

	/* CP_DYN_PWR=1 */
	codec_write_reg(dev, WM8904_CLS_W_0, 0x0001);

	/* CP_ENA=1 */
	codec_write_reg(dev, WM8904_CHRG_PUMP_0, 0x0001);

	codec_protol_config(dev, cfg->dai_type);
	codec_update_reg(dev, WM8904_CLK_RATES_2,
		(uint16_t)(1UL << 14U), (uint16_t)(dev_cfg->clock_source));

	sysclk = CLOCK_GetMclkClkFreq();
	cfg->mclk_freq = sysclk;

	codec_audio_fmt_config(dev, &cfg->dai_cfg, sysclk);

	if ((cfg->dai_cfg.i2s.options & I2S_OPT_FRAME_CLK_MASTER) ==
		I2S_OPT_FRAME_CLK_MASTER) {
		wm8904_set_master_clock(dev, &cfg->dai_cfg, sysclk);
	} else {
		/* BCLK/LRCLK default direction input */
		codec_update_reg(dev, WM8904_AUDIO_IF_1, 1U << 6U, 0U);
		codec_update_reg(dev, WM8904_AUDIO_IF_3, (uint16_t)(1UL << 11U), 0U);
	}
	codec_configure_output(dev);

	return 0;
}

static void codec_start_output(const struct device *dev)
{
}

static void codec_stop_output(const struct device *dev)
{
}

static int codec_set_property(const struct device *dev,
			      audio_property_t property,
			      audio_channel_t channel,
			      audio_property_value_t val)
{
	return -EINVAL;
}

static int codec_apply_properties(const struct device *dev)
{
	/* nothing to do because there is nothing cached */
	return 0;
}

static void codec_write_reg(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct codec_driver_config *const dev_cfg = DEV_CFG(dev);
	uint8_t data[3];
	int ret;

	/* data is reversed */
	data[0] = reg;
	data[1] = (val >> 8) & 0xff;
	data[2] = val & 0xff;

	ret = i2c_write(dev_cfg->i2c.bus, data, 3, dev_cfg->i2c.addr);

	if (ret != 0) {
		LOG_ERR("i2c write to codec error %d", ret);
	}

	LOG_DBG("REG:%02u VAL:0x%02x", reg, val);
}

static void codec_read_reg(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct codec_driver_config *const dev_cfg = DEV_CFG(dev);
	uint16_t value;
	int ret;

	ret = i2c_write_read(dev_cfg->i2c.bus, dev_cfg->i2c.addr, &reg,
		sizeof(reg), &value, sizeof(value));
	if (ret == 0) {
		*val = (value>>8)&0xff;
		*val += ((value & 0xff)<<8);
		/* update cache*/
		LOG_DBG("REG:%02u VAL:0x%02x", reg, *val);
	}
}

static void codec_update_reg(const struct device *dev, uint8_t reg,
			     uint16_t mask, uint16_t val)
{
	uint16_t reg_val = 0;
	uint16_t new_value = 0;

	if (reg == 0x19) {
		LOG_INF("try write mask 0x%x val 0x%x", mask, val);
	}
	codec_read_reg(dev, reg, &reg_val);
	if (reg == 0x19) {
		LOG_INF("read 0x%x = %x", reg, reg_val);
	}
	new_value = (reg_val & ~mask) | (val & mask);
	if (reg == 0x19) {
		LOG_INF("write 0x%x = %x", reg, new_value);
	}
	codec_write_reg(dev, reg, new_value);
}

static void codec_soft_reset(const struct device *dev)
{
	/* soft reset the DAC */
	codec_write_reg(dev, WM8904_RESET, 0x00);
}

static void codec_configure_output(const struct device *dev)
{
	uint16_t regValue = 0U;
	uint16_t regBitMask = 0xFU;

	/* source from DAC*/
	regValue &= (uint16_t) ~((3U << 2U) | 3U);
	codec_update_reg(dev, WM8904_ANALOG_OUT12_ZC, regBitMask, regValue);
	codec_update_reg(dev, WM8904_ANALOG_OUT1_LEFT, 0x1BF, 19 | 0x80U);
	codec_update_reg(dev, WM8904_ANALOG_OUT1_RIGHT, 0x1BF, 19 | 0x80U);
	/* Set unmute */
	codec_update_reg(dev, WM8904_ANALOG_OUT1_LEFT, 0x100U, 0X80U);
	codec_update_reg(dev, WM8904_ANALOG_OUT1_RIGHT, 0x100U, 0x80U);
	codec_update_reg(dev, WM8904_ANALOG_OUT2_LEFT, 0x100U, 0X80U);
	codec_update_reg(dev, WM8904_ANALOG_OUT2_RIGHT, 0x100U, 0x80U);
}

static const struct audio_codec_api codec_driver_api = {
	.configure = codec_configure,
	.start_output = codec_start_output,
	.stop_output = codec_stop_output,
	.set_property = codec_set_property,
	.apply_properties = codec_apply_properties,
};

#define WM8904_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);					\
	static const struct codec_driver_config codec_device_config_##n = {	\
		.i2c = I2C_DT_SPEC_INST_GET(n),				\
		.clock_source =	DT_INST_PROP_OR(n, clk_source, 0),	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static struct codec_driver_data codec_driver_data_##n = {		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &codec_initialize,	\
			      NULL,			\
			      &codec_driver_data_##n,	\
			      &codec_device_config_##n,	\
			      POST_KERNEL,		\
			      CONFIG_AUDIO_CODEC_INIT_PRIORITY,	\
			      &codec_driver_api);	\

DT_INST_FOREACH_STATUS_OKAY(WM8904_INIT)
