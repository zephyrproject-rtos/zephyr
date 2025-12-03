/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/audio/codec.h>
#include <zephyr/devicetree/clocks.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "da7212.h"

LOG_MODULE_REGISTER(dlg_da7212, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define DT_DRV_COMPAT dlg_da7212
#define DEV_CFG(dev)  ((const struct da7212_driver_config *const)dev->config)

struct da7212_driver_config {
	struct i2c_dt_spec i2c;
	int clock_source;
	const struct device *mclk_dev;
	clock_control_subsys_t mclk_name;
};

static inline void da7212_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct da7212_driver_config *const dev_cfg = DEV_CFG(dev);
	uint8_t data[2] = {reg, val};
	int ret;

	ret = i2c_write(dev_cfg->i2c.bus, data, sizeof(data), dev_cfg->i2c.addr);

	if (ret != 0) {
		LOG_ERR("i2c write to codec error %d (reg 0x%02x)", ret, reg);
	}

	LOG_DBG("REG:%02u VAL:%#02x", reg, val);
}

static inline void da7212_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct da7212_driver_config *const dev_cfg = DEV_CFG(dev);

	int ret;

	ret = i2c_write_read(dev_cfg->i2c.bus, dev_cfg->i2c.addr,
			&reg, sizeof(reg), val, sizeof(*val));

	if (ret != 0) {
		LOG_ERR("i2c read from codec error %d (reg 0x%02x)", ret, reg);
	} else {
		LOG_DBG("REG:%02u VAL:%#02x", reg, *val);
	}
}

static inline void da7212_update_reg(const struct device *dev, uint8_t reg,
						uint8_t mask, uint8_t val)
{
	uint8_t cur, newv;

	da7212_read_reg(dev, reg, &cur);
	LOG_DBG("read %#x = %x", reg, cur);

	/* Apply mask to update only selected bits */
	newv = (cur & (uint8_t)~mask) | (val & mask);

	LOG_DBG("write %#x = %x", reg, newv);
	da7212_write_reg(dev, reg, newv);
}

static void da7212_soft_reset(const struct device *dev)
{
	da7212_write_reg(dev, DIALOG7212_CIF_CTRL,
			(uint8_t)DIALOG7212_CIF_CTRL_CIF_REG_SOFT_RESET_MASK);
}

static int da7212_clock_mode_config(const struct device *dev, audio_dai_cfg_t *cfg)
{
	uint8_t val = 0;

	/* Master mode => DAI_CLK_EN = 1 (BCLK/WCLK output).
	 * Slave mode => DAI_CLK_EN = 0 (BCLK/WCLK input)
	 */
	if ((cfg->i2s.options & I2S_OPT_FRAME_CLK_SLAVE) == 0) {
		da7212_update_reg(dev, DIALOG7212_DAI_CLK_MODE,
				DIALOG7212_DAI_CLK_EN_MASK, DIALOG7212_DAI_CLK_EN_MASK);

		/* DAI master mode BCLK number per WCLK period */
		switch (cfg->i2s.word_size) {
		case 16:
			val = DIALOG7212_DAI_BCLKS_PER_WCLK_BCLK32;
			break;
		case 32:
			val = DIALOG7212_DAI_BCLKS_PER_WCLK_BCLK64;
			break;
		case 64:
			val = DIALOG7212_DAI_BCLKS_PER_WCLK_BCLK128;
			break;
		case 128:
			val = DIALOG7212_DAI_BCLKS_PER_WCLK_BCLK256;
			break;
		default:
			LOG_ERR("Word size %d not supported", cfg->i2s.word_size);
			return -EINVAL;
		}

		da7212_update_reg(dev, DIALOG7212_DAI_CLK_MODE,
				(uint8_t)DIALOG7212_DAI_BCLKS_PER_WCLK_MASK, val);
	} else {
		da7212_update_reg(dev, DIALOG7212_DAI_CLK_MODE,
					DIALOG7212_DAI_CLK_EN_MASK, 0);
	}

	return 0;
}

static int da7212_dac_input_config(const struct device *dev, audio_route_t route)
{
	if ((route == AUDIO_ROUTE_PLAYBACK) || (route == AUDIO_ROUTE_PLAYBACK_CAPTURE)) {
		/* Route DAI input to DAC outputs (playback path) */
		da7212_write_reg(dev, DIALOG7212_DIG_ROUTING_DAC,
				(uint8_t)(DIALOG7212_DIG_ROUTING_DAC_R_RSC_DAC_R |
					DIALOG7212_DIG_ROUTING_DAC_L_RSC_DAC_L));
	} else {
		/* Route ADC input to DAC outputs (bypass path) */
		da7212_write_reg(dev, DIALOG7212_DIG_ROUTING_DAC,
				(uint8_t)(DIALOG7212_DIG_ROUTING_DAC_R_RSC_ADC_R_OUTPUT |
					DIALOG7212_DIG_ROUTING_DAC_L_RSC_ADC_L_OUTPUT));
	}

	return 0;
}

static int da7212_protocol_config(const struct device *dev, audio_dai_type_t dai_type)
{
	uint8_t proto;

	switch (dai_type) {
	case AUDIO_DAI_TYPE_I2S:
		proto = DIALOG7212_DAI_FORMAT_I2S_MODE;
		break;
	case AUDIO_DAI_TYPE_LEFT_JUSTIFIED:
		proto = DIALOG7212_DAI_FORMAT_LEFT_JUSTIFIED;
		break;
	case AUDIO_DAI_TYPE_RIGHT_JUSTIFIED:
		proto = DIALOG7212_DAI_FORMAT_RIGHT_JUSTIFIED;
		break;
	case AUDIO_DAI_TYPE_PCMA:
		proto = DIALOG7212_DAI_FORMAT_DSP_MODE; /* Map to DSP mode */
		break;
	case AUDIO_DAI_TYPE_PCMB:
		proto = DIALOG7212_DAI_FORMAT_DSP_MODE; /* Map to DSP mode */
		break;
	default:
		return -EINVAL;
	}

	/* Keep DAI enabled flag set */
	da7212_update_reg(dev, DIALOG7212_DAI_CTRL,
			(uint8_t)(DIALOG7212_DAI_FORMAT_MASK), proto);
	LOG_DBG("Codec protocol: %#x", proto);

	return 0;
}

static int da7212_audio_format_config(const struct device *dev, audio_dai_cfg_t *cfg)
{
	uint8_t val = 0;

	/* Sample rate */
	switch (cfg->i2s.frame_clk_freq) {
	case 8000:
		val = DIALOG7212_SR_8KHZ;
		break;
	case 11025:
		val = DIALOG7212_SR_11_025KHZ;
		break;
	case 12000:
		val = DIALOG7212_SR_12KHZ;
		break;
	case 16000:
		val = DIALOG7212_SR_16KHZ;
		break;
	case 22050:
		val = DIALOG7212_SR_22KHZ;
		break;
	case 24000:
		val = DIALOG7212_SR_24KHZ;
		break;
	case 32000:
		val = DIALOG7212_SR_32KHZ;
		break;
	case 44100:
		val = DIALOG7212_SR_44_1KHZ;
		break;
	case 48000:
		val = DIALOG7212_SR_48KHZ;
		break;
	case 88200:
		val = DIALOG7212_SR_88_2KHZ;
		break;
	case 96000:
		val = DIALOG7212_SR_96KHZ;
		break;
	default:
		LOG_WRN("Invalid codec sample rate: %d", cfg->i2s.frame_clk_freq);
		return -EINVAL;
	}
	da7212_write_reg(dev, DIALOG7212_SR, val);

	/* Word length */
	switch (cfg->i2s.word_size) {
	case 16:
		val = DIALOG7212_DAI_WORD_LENGTH_16B;
		break;
	case 20:
		val = DIALOG7212_DAI_WORD_LENGTH_20B;
		break;
	case 24:
		val = DIALOG7212_DAI_WORD_LENGTH_24B;
		break;
	case 32:
		val = DIALOG7212_DAI_WORD_LENGTH_32B;
		break;
	default:
		LOG_ERR("Word size %d not supported", cfg->i2s.word_size);
		return -EINVAL;
	}
	da7212_update_reg(dev, DIALOG7212_DAI_CTRL,
			(uint8_t)DIALOG7212_DAI_WORD_LENGTH_MASK, val);

	return 0;
}

static int da7212_out_update(const struct device *dev, audio_channel_t channel,
			uint8_t reg, uint8_t val)
{
	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		da7212_write_reg(dev, reg, val);
		return 0;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		da7212_write_reg(dev, reg + 1U, val); /* R gain is next register for HP */
		return 0;
	case AUDIO_CHANNEL_ALL:
		da7212_write_reg(dev, reg, val);
		da7212_write_reg(dev, reg + 1U, val);
		return 0;
	default:
		return -EINVAL;
	}
}

static int da7212_out_volume_config(const struct device *dev,
			audio_channel_t channel, int volume)
{
	uint8_t vol = (uint8_t)CLAMP(volume, 0, DIALOG7212_HP_L_AMP_GAIN_STATUS_MASK);

	/* DIALOG7212_HP_L_GAIN at 0x48, DIALOG7212_HP_R_GAIN at 0x49 */
	return da7212_out_update(dev, channel, DIALOG7212_HP_L_GAIN, vol);
}

/* Mute by setting MUTE bit, keep HP_L_AMP_EN bit set */
static int da7212_out_mute_config(const struct device *dev,
				audio_channel_t channel, bool mute)
{
	/* Keep amp enabled while toggling mute:
	 * 0xC0 (EN|MUTE) to mute
	 * 0x80 (EN) to unmute
	 */
	uint8_t regValue = mute ? DIALOG7212_MUTE_MASK : DIALOG7212_UNMUTE_MASK;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		da7212_update_reg(dev, DIALOG7212_HP_L_CTRL, DIALOG7212_MUTE_MASK, regValue);
		return 0;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		da7212_update_reg(dev, DIALOG7212_HP_R_CTRL, DIALOG7212_MUTE_MASK, regValue);
		return 0;
	case AUDIO_CHANNEL_ALL:
		da7212_update_reg(dev, DIALOG7212_HP_L_CTRL, DIALOG7212_MUTE_MASK, regValue);
		da7212_update_reg(dev, DIALOG7212_HP_R_CTRL, DIALOG7212_MUTE_MASK, regValue);
		da7212_update_reg(dev, DIALOG7212_LINE_CTRL, DIALOG7212_MUTE_MASK, regValue);
		return 0;
	default:
		return -EINVAL;
	}
}

static int da7212_in_update(const struct device *dev, audio_channel_t channel,
			uint8_t reg_gain, uint8_t gain)
{
	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		da7212_write_reg(dev, reg_gain, gain);
		return 0;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		da7212_write_reg(dev, reg_gain + 1U, gain);
		return 0;
	case AUDIO_CHANNEL_ALL:
		da7212_write_reg(dev, reg_gain, gain);
		da7212_write_reg(dev, reg_gain + 1U, gain);
		return 0;
	default:
		return -EINVAL;
	}
}

static int da7212_in_volume_config(const struct device *dev,
			audio_channel_t channel, int volume)
{
	uint8_t vol = (uint8_t)CLAMP(volume, 0, DIALOG7212_ADC_L_GAIN_STATUS_MASK);

	/* DIALOG7212_ADC_L_GAIN at 0x36, DIALOG7212_ADC_R_GAIN at 0x37 */
	return da7212_in_update(dev, channel, DIALOG7212_ADC_L_GAIN, vol);
}

static int da7212_in_mute_config(const struct device *dev,
				audio_channel_t channel, bool mute)
{
	uint8_t reg = (channel == AUDIO_CHANNEL_FRONT_RIGHT) ?
			DIALOG7212_ADC_R_CTRL : DIALOG7212_ADC_L_CTRL;

	/* Keep ADC enabled while toggling mute:
	 * 0xC0 (EN|MUTE) to mute
	 * 0x80 (EN) to unmute
	 */
	uint8_t regValue = mute ? DIALOG7212_MUTE_MASK : DIALOG7212_UNMUTE_MASK;

	if (channel == AUDIO_CHANNEL_ALL) {
		da7212_update_reg(dev, DIALOG7212_ADC_L_CTRL, DIALOG7212_MUTE_MASK, regValue);
		da7212_update_reg(dev, DIALOG7212_ADC_R_CTRL, DIALOG7212_MUTE_MASK, regValue);
		return 0;
	}

	da7212_update_reg(dev, reg, DIALOG7212_MUTE_MASK, regValue);

	return 0;
}

static int da7212_route_input(const struct device *dev,
			audio_channel_t channel, uint32_t input)
{
	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		da7212_write_reg(dev, DIALOG7212_MIXIN_L_SELECT,
				DIALOG7212_MIXIN_L_SELECT_AUX_L_SEL_MASK);
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		da7212_write_reg(dev, DIALOG7212_MIXIN_R_SELECT,
				DIALOG7212_MIXIN_R_SELECT_AUX_R_SEL_MASK);
		break;
	case AUDIO_CHANNEL_ALL:
		da7212_write_reg(dev, DIALOG7212_MIXIN_L_SELECT,
				DIALOG7212_MIXIN_L_SELECT_AUX_L_SEL_MASK);
		da7212_write_reg(dev, DIALOG7212_MIXIN_R_SELECT,
				DIALOG7212_MIXIN_R_SELECT_AUX_R_SEL_MASK);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline void da7212_route_dac_to_mixout(const struct device *dev)
{
	da7212_write_reg(dev, DIALOG7212_MIXOUT_L_SELECT,
		(uint8_t)DIALOG7212_MIXOUT_L_SELECT_DAC_L_MASK);
	da7212_write_reg(dev, DIALOG7212_MIXOUT_R_SELECT,
		(uint8_t)DIALOG7212_MIXOUT_R_SELECT_DAC_R_MASK);
}

static int da7212_route_output(const struct device *dev,
			audio_channel_t channel, uint32_t output)
{
	/* Route DACs to mixers by default */
	ARG_UNUSED(output);

	da7212_route_dac_to_mixout(dev);

	return 0;
}

static void da7212_configure_output(const struct device *dev)
{
	/* Power charge pump */
	da7212_write_reg(dev, DIALOG7212_CP_CTRL,
		(uint8_t)(DIALOG7212_CP_CTRL_EN_MASK |
			DIALOG7212_CP_CTRL_SMALL_SWIT_CH_FREQ_EN_MASK |
			DIALOG7212_CP_CTRL_MCHANGE_OUTPUT |
			DIALOG7212_CP_CTRL_MOD_CPVDD_1 |
			DIALOG7212_CP_CTRL_ANALOG_VLL_LV_BOOSTS_CP));

	/* Route DAC to MixOut */
	da7212_route_dac_to_mixout(dev);

	/* Enable DACs with ramp */
	da7212_write_reg(dev, DIALOG7212_DAC_L_CTRL,
		(uint8_t)(DIALOG7212_DAC_L_CTRL_DAC_EN_MASK |
			DIALOG7212_DAC_L_CTRL_DAC_RAMP_EN_MASK));
	da7212_write_reg(dev, DIALOG7212_DAC_R_CTRL,
		(uint8_t)(DIALOG7212_DAC_R_CTRL_DAC_EN_MASK |
			DIALOG7212_DAC_R_CTRL_DAC_RAMP_EN_MASK));

	/* Enable HP amps, ZC and OE */
	da7212_write_reg(dev, DIALOG7212_HP_L_CTRL,
		(uint8_t)(DIALOG7212_HP_L_CTRL_AMP_EN_MASK |
			DIALOG7212_HP_L_CTRL_AMP_RAMP_EN_MASK |
			DIALOG7212_HP_L_CTRL_AMP_ZC_EN_MASK |
			DIALOG7212_HP_L_CTRL_AMP_OE_MASK));
	da7212_write_reg(dev, DIALOG7212_HP_R_CTRL,
		(uint8_t)(DIALOG7212_HP_R_CTRL_AMP_EN_MASK |
			DIALOG7212_HP_R_CTRL_AMP_RAMP_EN_MASK |
			DIALOG7212_HP_R_CTRL_AMP_ZC_EN_MASK |
			DIALOG7212_HP_R_CTRL_AMP_OE_MASK));

	/* Enable MixOut amplifiers and mixing into HP */
	da7212_write_reg(dev, DIALOG7212_MIXOUT_L_CTRL,
		(uint8_t)(DIALOG7212_MIXOUT_L_CTRL_AMP_EN_MASK |
			DIALOG7212_MIXOUT_L_CTRL_AMP_SOFT_MIX_EN_MASK |
			DIALOG7212_MIXOUT_L_CTRL_AMP_MIX_EN_MASK));
	da7212_write_reg(dev, DIALOG7212_MIXOUT_R_CTRL,
		(uint8_t)(DIALOG7212_MIXOUT_R_CTRL_AMP_EN_MASK |
			DIALOG7212_MIXOUT_R_CTRL_AMP_SOFT_MIX_EN_MASK |
			DIALOG7212_MIXOUT_R_CTRL_AMP_MIX_EN_MASK));

	/* Configure DAC gain to 0x67. */
	da7212_write_reg(dev, DIALOG7212_DAC_L_GAIN, (uint8_t)DIALOG7212_DAC_DEFAULT_GAIN);
	da7212_write_reg(dev, DIALOG7212_DAC_R_GAIN, (uint8_t)DIALOG7212_DAC_DEFAULT_GAIN);

	/* Set default HP volume and unmute */
	da7212_out_volume_config(dev, AUDIO_CHANNEL_ALL, DIALOG7212_HP_DEFAULT_GAIN);
	da7212_out_mute_config(dev, AUDIO_CHANNEL_ALL, false);
}

static void da7212_configure_input(const struct device *dev)
{
	/* Route AUX to MIXIN L/R (0x01 on both) */
	da7212_write_reg(dev, DIALOG7212_MIXIN_L_SELECT,
		(uint8_t)DIALOG7212_MIXIN_L_SELECT_AUX_L_SEL_MASK);
	da7212_write_reg(dev, DIALOG7212_MIXIN_R_SELECT,
		(uint8_t)DIALOG7212_MIXIN_R_SELECT_AUX_R_SEL_MASK);

	/* Charge pump control: 0xFD */
	da7212_write_reg(dev, DIALOG7212_CP_CTRL,
		(uint8_t)(DIALOG7212_CP_CTRL_EN_MASK |
			DIALOG7212_CP_CTRL_SMALL_SWIT_CH_FREQ_EN_MASK |
			DIALOG7212_CP_CTRL_MCHANGE_OUTPUT |
			DIALOG7212_CP_CTRL_MOD_CPVDD_1 |
			DIALOG7212_CP_CTRL_ANALOG_VLL_LV_BOOSTS_CP));

	/* AUX_L_CTRL: 0xB4 */
	da7212_write_reg(dev, DIALOG7212_AUX_L_CTRL,
		(uint8_t)(DIALOG7212_AUX_L_CTRL_AMP_EN_MASK |
			DIALOG7212_AUX_L_CTRL_AMP_RAMP_EN_MASK |
			DIALOG7212_AUX_L_CTRL_AMP_ZC_EN_MASK |
			DIALOG7212_AUX_L_CTRL_AMP_ZC_SEL_INPUT_AUX_L_IF));

	/* AUX_R_CTRL: 0xB0 */
	da7212_write_reg(dev, DIALOG7212_AUX_R_CTRL,
		(uint8_t)(DIALOG7212_AUX_R_CTRL_AMP_EN_MASK |
			DIALOG7212_AUX_R_CTRL_AMP_RAMP_EN_MASK |
			DIALOG7212_AUX_R_CTRL_AMP_ZC_EN_MASK));

	/* MIC_1_CTRL: 0x04 */
	da7212_write_reg(dev, DIALOG7212_MIC_1_CTRL,
		(uint8_t)DIALOG7212_MIC_1_CTRL_AMP_IN_SEL_MIC_1_P);
	da7212_write_reg(dev, DIALOG7212_MIC_2_CTRL,
		(uint8_t)DIALOG7212_MIC_2_CTRL_AMP_IN_SEL_MIC_2_P);

	/* MIXIN_L/R_CTRL: 0x88 */
	da7212_write_reg(dev, DIALOG7212_MIXIN_L_CTRL,
		(uint8_t)(DIALOG7212_MIXIN_L_CTRL_AMP_EN_MASK |
			DIALOG7212_MIXIN_L_CTRL_AMP_MIX_EN_MASK));
	da7212_write_reg(dev, DIALOG7212_MIXIN_R_CTRL,
		(uint8_t)(DIALOG7212_MIXIN_R_CTRL_AMP_EN_MASK |
			DIALOG7212_MIXIN_R_CTRL_AMP_MIX_EN_MASK));

	/* ADC_L_CTRL: 0xA0 */
	da7212_write_reg(dev, DIALOG7212_ADC_L_CTRL,
		(uint8_t)(DIALOG7212_ADC_L_CTRL_ADC_EN_MASK |
			DIALOG7212_ADC_L_CTRL_ADC_RAMP_EN_MASK));
	da7212_write_reg(dev, DIALOG7212_ADC_R_CTRL,
		(uint8_t)(DIALOG7212_ADC_R_CTRL_ADC_EN_MASK |
			DIALOG7212_ADC_R_CTRL_ADC_RAMP_EN_MASK));

	/* GAIN_RAMP_CTRL: 0x02 */
	da7212_write_reg(dev, DIALOG7212_GAIN_RAMP_CTRL,
		(uint8_t)DIALOG7212_GAIN_RAMP_CTRL_RATE_NR_MUL_16);

	/* PC_COUNT: 0x02 */
	da7212_write_reg(dev, DIALOG7212_PC_COUNT,
		(uint8_t)DIALOG7212_PC_COUNT_RESYNC_MASK);

	/* CP_DELAY: 0x95 */
	da7212_write_reg(dev, DIALOG7212_CP_DELAY,
		(uint8_t)(DIALOG7212_CP_DELAY_ON_OFF_LIMITER_AUT |
			DIALOG7212_CP_DELAY_TAU_DELAY_4MS |
			DIALOG7212_CP_DELAY_FCONTROL_0HZ_OR_1MHZ));

	/* Set default ADC volume and unmute */
	da7212_in_volume_config(dev, AUDIO_CHANNEL_ALL, DIALOG7212_HP_DEFAULT_GAIN);
	da7212_in_mute_config(dev, AUDIO_CHANNEL_ALL, false);
}

static int da7212_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	const struct da7212_driver_config *const dev_cfg = DEV_CFG(dev);

	if (cfg->dai_type >= AUDIO_DAI_TYPE_INVALID) {
		LOG_ERR("dai_type not supported");
		return -EINVAL;
	}

	if (cfg->dai_route == AUDIO_ROUTE_BYPASS) {
		return 0;
	}

	if (dev_cfg->clock_source == 0) {
		int err = clock_control_on(dev_cfg->mclk_dev, dev_cfg->mclk_name);

		if (err < 0) {
			LOG_ERR("MCLK clock source enable fail: %d", err);
		}
		err = clock_control_get_rate(dev_cfg->mclk_dev, dev_cfg->mclk_name,
				&cfg->mclk_freq);
		if (err < 0) {
			LOG_ERR("MCLK clock source freq acquire fail: %d", err);
		}
	}

	da7212_soft_reset(dev);

	/* DAI right/left output stream comes from ADC right/left.
	 * Not used in AUDIO_ROUTE_PLAYBACK routing.
	 */
	da7212_write_reg(dev, DIALOG7212_DIG_ROUTING_DAI,
			(uint8_t)(DIALOG7212_DIG_ROUTING_DAI_R_SRC_ADC_RIGHT |
				DIALOG7212_DIG_ROUTING_DAI_L_SRC_ADC_LEFT));

	/* Set default sample rate to 16kHz */
	da7212_write_reg(dev, DIALOG7212_SR,
			(uint8_t)DIALOG7212_SR_16KHZ);

	/* Enable voltage reference and bias */
	da7212_write_reg(dev, DIALOG7212_REFERENCES,
			(uint8_t)DIALOG7212_REFERENCES_BIAS_EN_MASK);

	/* Keep PLL disable, use MCLK as system clock. */
	da7212_write_reg(dev, DIALOG7212_PLL_FRAC_TOP, 0);
	da7212_write_reg(dev, DIALOG7212_PLL_FRAC_BOT, 0);
	da7212_write_reg(dev, DIALOG7212_PLL_INTEGER,
			DIALOG7212_PLL_FBDIV_INTEGER_RESET_VALUE);
	da7212_write_reg(dev, DIALOG7212_PLL_CTRL, 0x0);

	/* Set default clock mode to slave, BCLK number per WCLK = 64 */
	da7212_write_reg(dev, DIALOG7212_DAI_CLK_MODE,
			(uint8_t)DIALOG7212_DAI_BCLKS_PER_WCLK_BCLK64);

	/* Enable DAI, set default word length to 16 bits, I2S format,
	 * output enabled
	 */
	da7212_write_reg(dev, DIALOG7212_DAI_CTRL,
			(uint8_t)(DIALOG7212_DAI_EN_MASK |
				DIALOG7212_DAI_OE_MASK |
				DIALOG7212_DAI_WORD_LENGTH_16B |
				DIALOG7212_DAI_FORMAT_I2S_MODE));

	/* Route DAC to MixOut by default */
	da7212_write_reg(dev, DIALOG7212_DIG_ROUTING_DAC,
			(uint8_t)(DIALOG7212_DIG_ROUTING_DAC_R_RSC_DAC_R |
				DIALOG7212_DIG_ROUTING_DAC_L_RSC_DAC_L));

	/* Clock mode configuration */
	da7212_clock_mode_config(dev, &cfg->dai_cfg);
	/* DAC input configuration */
	da7212_dac_input_config(dev, cfg->dai_route);
	/* Protocol configuration */
	da7212_protocol_config(dev, cfg->dai_type);
	/* Sample rate, word length configuration */
	da7212_audio_format_config(dev, &cfg->dai_cfg);

	switch (cfg->dai_route) {
	case AUDIO_ROUTE_PLAYBACK:
		da7212_configure_output(dev);
		break;
	case AUDIO_ROUTE_CAPTURE:
		da7212_configure_input(dev);
		break;
	case AUDIO_ROUTE_PLAYBACK_CAPTURE:
		da7212_configure_output(dev);
		da7212_configure_input(dev);
		break;
	default:
		break;
	}

	return 0;
}

static void da7212_start_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static void da7212_stop_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int da7212_set_property(const struct device *dev, audio_property_t property,
				audio_channel_t channel, audio_property_value_t val)
{
	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME:
		return da7212_out_volume_config(dev, channel, val.vol);
	case AUDIO_PROPERTY_OUTPUT_MUTE:
		return da7212_out_mute_config(dev, channel, val.mute);
	case AUDIO_PROPERTY_INPUT_VOLUME:
		return da7212_in_volume_config(dev, channel, val.vol);
	case AUDIO_PROPERTY_INPUT_MUTE:
		return da7212_in_mute_config(dev, channel, val.mute);
	default:
		break;
	}

	return -EINVAL;
}

static int da7212_apply_properties(const struct device *dev)
{
	/* Nothing special: gains written immediately */
	return 0;
}

static const struct audio_codec_api da7212_driver_api = {
	.configure = da7212_configure,
	.start_output = da7212_start_output,
	.stop_output = da7212_stop_output,
	.set_property = da7212_set_property,
	.apply_properties = da7212_apply_properties,
	.route_input = da7212_route_input,
	.route_output = da7212_route_output,
};

#define DA7212_INIT(n)									\
	static const struct da7212_driver_config da7212_device_config_##n = {		\
		.i2c = I2C_DT_SPEC_INST_GET(n),						\
		.clock_source = DT_INST_ENUM_IDX(n, clock_source),			\
		.mclk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(n, mclk)),	\
		.mclk_name = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_NAME(n,	\
								 mclk, name)};		\
											\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &da7212_device_config_##n,		\
		POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &da7212_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DA7212_INIT)
