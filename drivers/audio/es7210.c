/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Initialization and clock coefficients adapted from esp-bsp's ES7210
 * component, version 1.0.1~1 (f259a6abd5808019b1903a422092a4234ca95a51).
 * Mute control follows esp-adf (a8a14c8fa6cb8aefa5299f4c06fe9679278097cc).
 */

#define DT_DRV_COMPAT everest_es7210

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/audio/es7210.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "es7210.h"

LOG_MODULE_REGISTER(es7210, CONFIG_AUDIO_CODEC_LOG_LEVEL);

struct es7210_config {
	struct i2c_dt_spec i2c;
	uint16_t vdda_mv;
	uint16_t vddd_mv;
	uint8_t sdout_pair;
	uint8_t bias[2];
};

struct es7210_data {
	struct k_mutex lock;
	struct audio_codec_es7210_dai_cfg serial;
	struct audio_codec_es7210_processing processing;
	uint8_t wire_mask;
	uint8_t mute;
	bool configured;
	bool running;
};

struct es7210_reg {
	uint8_t addr;
	uint8_t value;
};

struct es7210_clock {
	uint8_t mainclk;
	uint8_t osr;
	uint16_t lrck_div;
	uint8_t bclk_div;
};

static uint8_t es7210_analog(const struct es7210_config *config)
{
	/* Revision 21.0 register 0x40: VX1SEL at 1.8 V, VX2OFF at 3.3 V. */
	return config->vdda_mv == 1800U ? ES7210_ANALOG_VX1_SELECT : ES7210_ANALOG_VX2_OFF;
}

static int es7210_write(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct es7210_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, value);
}

static int es7210_write_sequence(const struct device *dev, const struct es7210_reg *regs,
				 size_t count)
{
	for (size_t i = 0; i < count; i++) {
		int ret = es7210_write(dev, regs[i].addr, regs[i].value);

		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

/* Attempt every shutdown write even when the bus or one register fails. */
static int es7210_quiesce(const struct device *dev)
{
	const struct es7210_config *config = dev->config;
	const struct es7210_reg regs[] = {
		{ES7210_MUTE12, 0x03U},
		{ES7210_MUTE34, 0x03U},
		{ES7210_POWER12, 0xffU},
		{ES7210_POWER34, 0xffU},
		{ES7210_ANALOG, ES7210_ANALOG_POWER_DOWN | es7210_analog(config)},
		{ES7210_CLOCK_OFF, 0x7fU},
	};
	int first_error = 0;

	for (size_t i = 0; i < ARRAY_SIZE(regs); i++) {
		int ret = es7210_write(dev, regs[i].addr, regs[i].value);

		if ((ret != 0) && (first_error == 0)) {
			first_error = ret;
		}
	}
	/* DLL power is retained: powering it down requires stopping external MCLK first. */
	return first_error;
}

static void es7210_fault(const struct device *dev)
{
	struct es7210_data *data = dev->data;
	int ret = es7210_quiesce(dev);

	data->configured = false;
	data->running = false;
	if (ret != 0) {
		LOG_ERR("Failed to shut down ADC: %d", ret);
	}
}

static bool es7210_rate_valid(uint32_t rate)
{
	switch (rate) {
	case 8000U:
	case 11025U:
	case 12000U:
	case 16000U:
	case 22050U:
	case 24000U:
	case 32000U:
	case 44100U:
	case 48000U:
	case 64000U:
	case 88200U:
	case 96000U:
		return true;
	default:
		return false;
	}
}

static int es7210_clock_config(const struct audio_codec_es7210_dai_cfg *cfg,
			       struct es7210_clock *clock_cfg)
{
	/* 19.2 MHz coefficients from esp-bsp; omit its ambiguous 64 kHz speed-mode entry. */
	static const struct {
		uint32_t rate;
		uint8_t mainclk;
		uint8_t osr;
	} usb_clocks[] = {
		{8000U, 0x5eU, 0x28U},
		{12000U, 0x54U, 0x28U},
		{16000U, 0x0aU, 0x1eU},
		{24000U, 0x4aU, 0x28U},
		{32000U, 0x05U, 0x1eU},
		{48000U, 0x45U, 0x28U},
		{96000U, 0x45U, 0x28U},
	};
	static const uint8_t multipliers[] = {1U, 2U, 4U, 8U};
	static const uint8_t multiplier_bits[] = {0x80U, 0xc0U, 0x00U, 0x40U};
	uint32_t ratio;
	uint32_t adc_ratio = cfg->sample_rate > 48000U ? 256U : 512U;

	if (!es7210_rate_valid(cfg->sample_rate) || (cfg->mclk_freq == 0U) ||
	    (cfg->mclk_freq > 49200000U) || ((cfg->mclk_freq % cfg->sample_rate) != 0U)) {
		return -ENOTSUP;
	}
	ratio = cfg->mclk_freq / cfg->sample_rate;
	if ((ratio == 0U) || (ratio > 4095U)) {
		return -ENOTSUP;
	}
	clock_cfg->lrck_div = ratio;
	clock_cfg->bclk_div = 0U;
	if (cfg->clock_controller) {
		uint32_t frame_bits = cfg->channels * cfg->slot_width;

		if (((ratio % frame_bits) != 0U) || ((ratio / frame_bits) > 127U)) {
			return -ENOTSUP;
		}
		clock_cfg->bclk_div = ratio / frame_bits;
	}
	if (cfg->mclk_freq == 19200000U) {
		for (size_t i = 0; i < ARRAY_SIZE(usb_clocks); i++) {
			if (cfg->sample_rate == usb_clocks[i].rate) {
				clock_cfg->mainclk = usb_clocks[i].mainclk;
				clock_cfg->osr = usb_clocks[i].osr;
				return 0;
			}
		}
		return -ENOTSUP;
	}

	/* Standard audio ratios, extending the esp-bsp OSR=32 coefficients. */
	switch (ratio) {
	case 64U:
	case 128U:
	case 256U:
	case 384U:
	case 512U:
	case 768U:
	case 1024U:
	case 1536U:
	case 2048U:
	case 3072U:
		break;
	default:
		return -ENOTSUP;
	}
	for (size_t i = 0; i < ARRAY_SIZE(multipliers); i++) {
		uint32_t numerator = ratio * multipliers[i];
		uint32_t divider = numerator / adc_ratio;

		if (((numerator % adc_ratio) == 0U) && (divider >= 1U) && (divider <= 31U)) {
			clock_cfg->mainclk = multiplier_bits[i] | divider;
			clock_cfg->osr = 0x20U;
			return 0;
		}
	}
	return -ENOTSUP;
}

static int es7210_level_code(int16_t level)
{
	static const int16_t levels[] = {
		-301, -241, -206, -181, -161, -145, -132, -120,
		-110, -101, -93,  -85,  -78,  -72,  -66,  -60,
	};

	for (size_t i = 0; i < ARRAY_SIZE(levels); i++) {
		if (level == levels[i]) {
			return i;
		}
	}
	return -EINVAL;
}

static int es7210_pga_code(int16_t gain)
{
	if ((gain >= 0) && (gain <= 66) && ((gain % 6) == 0)) {
		return gain / 6;
	}
	switch (gain) {
	case 69:
		return 12;
	case 72:
		return 13;
	case 75:
		return 14;
	default:
		return -EINVAL;
	}
}

static uint8_t es7210_ramp_code(uint32_t cycles)
{
	uint8_t code = 0U;

	for (uint32_t n = cycles / 2U; n > 1U; n >>= 1U) {
		code++;
	}
	return code;
}

static int es7210_processing_validate(const struct device *dev,
				      const struct audio_codec_es7210_processing *cfg)
{
	const struct es7210_config *config = dev->config;
	struct es7210_data *data = dev->data;

	if ((cfg->mic_mask == 0U) || ((cfg->mic_mask & ~data->wire_mask) != 0U) ||
	    (cfg->alc_ramp_cycles < 2U) || (cfg->alc_ramp_cycles > 65536U) ||
	    !is_power_of_two(cfg->alc_ramp_cycles)) {
		return -EINVAL;
	}
	if ((cfg->gain_ramp_cycles != 0U) &&
	    ((cfg->gain_ramp_cycles < 4U) || (cfg->gain_ramp_cycles > 256U) ||
	     !is_power_of_two(cfg->gain_ramp_cycles))) {
		return -EINVAL;
	}
	for (uint8_t i = 0U; i < 4U; i++) {
		const struct audio_codec_es7210_channel_cfg *ch = &cfg->channel[i];

		if ((es7210_pga_code(ch->pga_gain) < 0) || (ch->digital_gain < -191) ||
		    (ch->digital_gain > 64)) {
			return -EINVAL;
		}
		if ((config->vdda_mv == 1800U) && !cfg->pair[i / 2U].digital_mic &&
		    ((cfg->mic_mask & BIT(i)) != 0U) && (ch->pga_gain < 42)) {
			return -EINVAL;
		}
	}
	for (uint8_t i = 0U; i < 2U; i++) {
		const struct audio_codec_es7210_pair_cfg *pair = &cfg->pair[i];

		if ((pair->route < AUDIO_CODEC_ES7210_ROUTE_NORMAL) ||
		    (pair->route > AUDIO_CODEC_ES7210_ROUTE_SWAP) ||
		    (pair->digital_mic_gain > 42U) || ((pair->digital_mic_gain % 6U) != 0U) ||
		    (pair->hpf_coefficient1 > 31U) || (pair->hpf_coefficient2 > 31U) ||
		    (pair->alc_channels > 3U) || (es7210_level_code(pair->alc_min_level) < 0) ||
		    (es7210_level_code(pair->alc_max_level) < 0) ||
		    (pair->alc_min_level > pair->alc_max_level)) {
			return -EINVAL;
		}
	}
	return 0;
}

static uint8_t es7210_microphone_gain(const struct audio_codec_es7210_processing *cfg, uint8_t ch)
{
	uint8_t select = 0U;

	if (((cfg->mic_mask & BIT(ch)) != 0U) && !cfg->pair[ch / 2U].digital_mic) {
		select = ES7210_MIC_SELECT;
	}
	return select | es7210_pga_code(cfg->channel[ch].pga_gain);
}

static int es7210_write_processing(const struct device *dev,
				   const struct audio_codec_es7210_processing *cfg)
{
	uint8_t dmic = 0U;
	uint8_t alc = 0U;
	int ret;

	for (uint8_t i = 0U; i < 4U; i++) {
		ret = es7210_write(dev, ES7210_MIC1_GAIN + i, es7210_microphone_gain(cfg, i));
		if (ret != 0) {
			return ret;
		}
		/* Revision 21.0: digital gain addresses run ADC4..ADC1, not ADC1..ADC4. */
		ret = es7210_write(dev, ES7210_ADC1_GAIN - i, cfg->channel[i].digital_gain + 191);
		if (ret != 0) {
			return ret;
		}
	}
	for (uint8_t i = 0U; i < 2U; i++) {
		const struct audio_codec_es7210_pair_cfg *pair = &cfg->pair[i];
		uint8_t reg1 = i == 0U ? ES7210_HPF12_1 : ES7210_HPF34_1;
		uint8_t reg2 = i == 0U ? ES7210_HPF12_2 : ES7210_HPF34_2;
		uint8_t hpf1 = pair->hpf_coefficient1 |
			       (pair->freeze_offset ? ES7210_HPF_FREEZE_OFFSET : 0U) |
			       (cfg->channel[i * 2U].invert ? ES7210_HPF_INVERT_FIRST : 0U) |
			       (cfg->channel[i * 2U + 1U].invert ? ES7210_HPF_INVERT_SECOND : 0U);
		uint8_t level = (es7210_level_code(pair->alc_max_level) << 4) |
				es7210_level_code(pair->alc_min_level);
		/* ALC selection encodes left as bit 1, unlike the physical mic mask. */
		uint8_t select =
			((pair->alc_channels & 1U) << 1) | ((pair->alc_channels & 2U) >> 1);
		const struct es7210_reg regs[] = {
			{reg1, hpf1},
			{reg2,
			 FIELD_PREP(ES7210_HPF_ROUTE_MASK, pair->route) | pair->hpf_coefficient2},
			{i == 0U ? ES7210_ALC_LEVEL12 : ES7210_ALC_LEVEL34, level},
		};

		ret = es7210_write_sequence(dev, regs, ARRAY_SIZE(regs));
		if (ret != 0) {
			return ret;
		}
		alc |= select << (i == 0U ? 2U : 0U);
		dmic |= (pair->digital_mic ? BIT(i == 0U ? 6 : 7) : 0U) |
			((pair->digital_mic_gain / 6U) << (i == 0U ? 0U : 3U));
	}
	const struct es7210_reg regs[] = {
		{ES7210_DMIC, dmic},
		{ES7210_ALC_RAMP, es7210_ramp_code(cfg->alc_ramp_cycles)},
		{ES7210_GAIN_RAMP, es7210_ramp_code(cfg->gain_ramp_cycles) << 4},
		{ES7210_ALC_SELECT, alc},
	};

	return es7210_write_sequence(dev, regs, ARRAY_SIZE(regs));
}

static void es7210_processing_defaults(const struct device *dev)
{
	const struct es7210_config *config = dev->config;
	struct es7210_data *data = dev->data;

	data->processing = (struct audio_codec_es7210_processing){
		.mic_mask = data->wire_mask,
		.alc_ramp_cycles = 2U,
	};
	for (uint8_t i = 0U; i < 4U; i++) {
		data->processing.channel[i].pga_gain = config->vdda_mv == 1800U ? 42 : 0;
	}
	for (uint8_t i = 0U; i < 2U; i++) {
		data->processing.pair[i].hpf_coefficient1 = 10U;
		data->processing.pair[i].hpf_coefficient2 = 10U;
		data->processing.pair[i].freeze_offset = true;
		data->processing.pair[i].alc_min_level = -120;
		data->processing.pair[i].alc_max_level = -60;
	}
}

static int es7210_configure_serial(const struct device *dev,
				   const struct audio_codec_es7210_dai_cfg *cfg)
{
	const struct es7210_config *config = dev->config;
	struct es7210_data *data = dev->data;
	struct es7210_clock clock_cfg;
	uint8_t word;
	uint8_t format;
	/* Bypass EQ: coefficient encoding is not publicly documented. */
	uint8_t mode = ES7210_MODE_EQ_BYPASS | ES7210_MODE_LRCK_RATE_DEFAULT;
	uint8_t output;
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}
	if ((cfg->channels != 2U) && (cfg->channels != 4U)) {
		return -ENOTSUP;
	}
	if ((cfg->slot_width < cfg->word_size) || (cfg->slot_width > 32U) ||
	    ((cfg->channels == 4U) &&
	     ((config->sdout_pair != 0U) || (cfg->slot_width != cfg->word_size)))) {
		return -ENOTSUP;
	}
	switch (cfg->word_size) {
	case 16U:
		word = FIELD_PREP(ES7210_SDP1_WORD_SIZE_MASK, 3U);
		break;
	case 18U:
		word = FIELD_PREP(ES7210_SDP1_WORD_SIZE_MASK, 2U);
		break;
	case 20U:
		word = FIELD_PREP(ES7210_SDP1_WORD_SIZE_MASK, 1U);
		break;
	case 24U:
		word = FIELD_PREP(ES7210_SDP1_WORD_SIZE_MASK, 0U);
		break;
	case 32U:
		word = FIELD_PREP(ES7210_SDP1_WORD_SIZE_MASK, 4U);
		break;
	default:
		return -ENOTSUP;
	}
	switch (cfg->format) {
	case AUDIO_CODEC_ES7210_I2S:
		format = 0U;
		break;
	case AUDIO_CODEC_ES7210_LEFT_JUSTIFIED:
		format = 1U;
		break;
	case AUDIO_CODEC_ES7210_DSP_A:
		format = 3U;
		break;
	case AUDIO_CODEC_ES7210_DSP_B:
		format = 0x13U;
		break;
	default:
		return -ENOTSUP;
	}
	if (cfg->invert_lrck) {
		if (cfg->format >= AUDIO_CODEC_ES7210_DSP_A) {
			return -ENOTSUP;
		}
		format |= ES7210_SDP1_LRCK_INVERT;
	}
	if ((config->vddd_mv == 1800U) && (cfg->sample_rate > 24000U)) {
		return -ENOTSUP;
	}
	ret = es7210_clock_config(cfg, &clock_cfg);
	if (ret != 0) {
		return ret;
	}
	mode |= cfg->clock_controller ? ES7210_MODE_CONTROLLER : 0U;
	mode |= cfg->sample_rate > 48000U ? ES7210_MODE_DOUBLE_SPEED : 0U;
	mode |= cfg->invert_bclk ? ES7210_MODE_BCLK_INVERT : 0U;
	output = config->sdout_pair == 0U ? ES7210_SDP2_SDOUT2_TRISTATE :
					  ES7210_SDP2_SDOUT1_TRISTATE;
	if (cfg->channels == 4U) {
		output |= cfg->format >= AUDIO_CODEC_ES7210_DSP_A ? ES7210_SDP2_TDM_DSP
								  : ES7210_SDP2_TDM_I2S;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	if (data->running) {
		ret = -EBUSY;
		goto out;
	}
	data->configured = false;
	data->serial = *cfg;
	data->wire_mask = cfg->channels == 4U ? 0x0fU : 3U << (config->sdout_pair * 2U);
	es7210_processing_defaults(dev);

	const struct es7210_reg regs[] = {
		{ES7210_RESET, 0xffU},
		{ES7210_RESET, 0x32U},
		{ES7210_CLOCK_OFF, 0x3fU},
		{ES7210_TIME_CONTROL0, 0x30U},
		{ES7210_TIME_CONTROL1, 0x30U},
		{ES7210_MODE, mode},
		{ES7210_MASTER_CLK, clock_cfg.bclk_div},
		{ES7210_LRCK_HIGH, clock_cfg.lrck_div >> 8},
		{ES7210_LRCK_LOW, clock_cfg.lrck_div & 0xffU},
		{ES7210_ANALOG, 0x83U | es7210_analog(config)},
		{ES7210_BIAS12, FIELD_PREP(ES7210_BIAS_VOLTAGE_MASK, config->bias[0])},
		{ES7210_BIAS34, FIELD_PREP(ES7210_BIAS_VOLTAGE_MASK, config->bias[1])},
		{ES7210_MAINCLK, clock_cfg.mainclk},
		{ES7210_OSR, clock_cfg.osr},
		{ES7210_POWER_DOWN, 0x00U},
		{ES7210_SDP1, word | format},
		{ES7210_SDP2, output},
	};

	ret = es7210_write_sequence(dev, regs, ARRAY_SIZE(regs));
	if (ret == 0) {
		ret = es7210_write_processing(dev, &data->processing);
	}
	if (ret == 0) {
		ret = es7210_quiesce(dev);
	}
	if (ret != 0) {
		es7210_fault(dev);
		goto out;
	}
	data->mute = 0U;
	data->configured = true;
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int es7210_configure(const struct device *dev, struct audio_codec_cfg *cfg)
{
	struct audio_codec_es7210_dai_cfg serial;
	uint8_t format;
	uint8_t options;

	if (cfg == NULL) {
		return -EINVAL;
	}
	if ((cfg->dai_route != AUDIO_ROUTE_CAPTURE) || (cfg->dai_type != AUDIO_DAI_TYPE_I2S)) {
		return -ENOTSUP;
	}
	format = cfg->dai_cfg.i2s.format;
	options = cfg->dai_cfg.i2s.options;
	if (((format & ~(I2S_FMT_DATA_FORMAT_MASK | I2S_FMT_CLK_FORMAT_MASK)) != 0U) ||
	    ((options & ~(I2S_OPT_BIT_CLK_TARGET | I2S_OPT_FRAME_CLK_TARGET | I2S_OPT_PINGPONG)) !=
	     0U) ||
	    (((options & I2S_OPT_BIT_CLK_TARGET) != 0U) !=
	     ((options & I2S_OPT_FRAME_CLK_TARGET) != 0U))) {
		return -ENOTSUP;
	}
	serial = (struct audio_codec_es7210_dai_cfg){
		.mclk_freq = cfg->mclk_freq,
		.sample_rate = cfg->dai_cfg.i2s.frame_clk_freq,
		.word_size = cfg->dai_cfg.i2s.word_size,
		.slot_width = cfg->dai_cfg.i2s.word_size > 16U ? 32U : 16U,
		.channels = cfg->dai_cfg.i2s.channels,
		.clock_controller = (options & I2S_OPT_BIT_CLK_TARGET) != 0U,
		.invert_bclk = (format & I2S_FMT_BIT_CLK_INV) != 0U,
		.invert_lrck = (format & I2S_FMT_FRAME_CLK_INV) != 0U,
	};
	switch (format & I2S_FMT_DATA_FORMAT_MASK) {
	case I2S_FMT_DATA_FORMAT_I2S:
		serial.format = AUDIO_CODEC_ES7210_I2S;
		break;
	case I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED:
		serial.format = AUDIO_CODEC_ES7210_LEFT_JUSTIFIED;
		break;
	default:
		return -ENOTSUP;
	}
	/* The generic I2S API fixes I2S frames to two words. Use the extension for TDM. */
	if (serial.channels != 2U) {
		return -ENOTSUP;
	}
	return es7210_configure_serial(dev, &serial);
}

static uint8_t es7210_pair_power(uint8_t mask)
{
	/* The automatic power sequence needs both converters of an active pair powered. */
	return mask != 0U ? 0x00U : 0xffU;
}

static int es7210_start(const struct device *dev, audio_dai_dir_t dir)
{
	const struct es7210_config *config = dev->config;
	struct es7210_data *data = dev->data;
	uint8_t clock_off;
	uint8_t effective_mute;
	int ret = 0;

	if (dir != AUDIO_DAI_DIR_RX) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->configured) {
		ret = -EIO;
		goto out;
	}
	if (data->running) {
		goto out;
	}
	clock_off = data->serial.clock_controller ? 0U : BIT(5);
	clock_off |= (data->processing.mic_mask & 3U) != 0U ? 0U : 0x0aU;
	clock_off |= (data->processing.mic_mask & 12U) != 0U ? 0U : 0x14U;
	effective_mute = data->mute | (uint8_t)~data->processing.mic_mask;
	const struct es7210_reg regs[] = {
		{ES7210_CLOCK_OFF, clock_off},
		{ES7210_ANALOG, 3U | es7210_analog(config)},
		{ES7210_MIC1_LOW_POWER, 0x08U},
		{ES7210_MIC1_LOW_POWER + 1U, 0x08U},
		{ES7210_MIC1_LOW_POWER + 2U, 0x08U},
		{ES7210_MIC1_LOW_POWER + 3U, 0x08U},
		{ES7210_POWER12, es7210_pair_power(data->processing.mic_mask & 3U)},
		{ES7210_POWER34, es7210_pair_power((data->processing.mic_mask >> 2) & 3U)},
		{ES7210_RESET, 0x71U},
		{ES7210_RESET, data->serial.clock_controller ? 0x01U : 0x41U},
		{ES7210_MUTE12, effective_mute & 3U},
		{ES7210_MUTE34, (effective_mute >> 2) & 3U},
	};

	ret = es7210_write_sequence(dev, regs, ARRAY_SIZE(regs));
	if (ret != 0) {
		es7210_fault(dev);
	} else {
		data->running = true;
	}
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int es7210_stop(const struct device *dev, audio_dai_dir_t dir)
{
	struct es7210_data *data = dev->data;
	int ret;

	if (dir != AUDIO_DAI_DIR_RX) {
		return -ENOTSUP;
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = es7210_quiesce(dev);
	data->running = false;
	if (ret != 0) {
		data->configured = false;
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

static int es7210_channel_mask(const struct device *dev, audio_channel_t channel)
{
	const struct es7210_config *config = dev->config;
	struct es7210_data *data = dev->data;
	uint8_t mask;
	uint8_t offset = data->serial.channels == 4U ? 0U : config->sdout_pair * 2U;

	switch ((uint32_t)channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		mask = BIT(offset);
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		mask = BIT(offset + 1U);
		break;
	case AUDIO_CHANNEL_ALL:
		mask = data->wire_mask;
		break;
	case AUDIO_CODEC_ES7210_MIC1:
		mask = BIT(0);
		break;
	case AUDIO_CODEC_ES7210_MIC2:
		mask = BIT(1);
		break;
	case AUDIO_CODEC_ES7210_MIC3:
		mask = BIT(2);
		break;
	case AUDIO_CODEC_ES7210_MIC4:
		mask = BIT(3);
		break;
	default:
		return -ENOTSUP;
	}
	return (mask & ~data->wire_mask) == 0U ? mask : -EINVAL;
}

static int es7210_set_property(const struct device *dev, audio_property_t property,
			       audio_channel_t channel, audio_property_value_t value)
{
	const struct es7210_config *config = dev->config;
	struct es7210_data *data = dev->data;
	int mask;
	int gain = 0;
	int ret = 0;

	if ((property != AUDIO_PROPERTY_INPUT_VOLUME) && (property != AUDIO_PROPERTY_INPUT_MUTE)) {
		return -ENOTSUP;
	}
	if (property == AUDIO_PROPERTY_INPUT_VOLUME) {
		if ((value.vol < 0) || (value.vol > 37)) {
			return -EINVAL;
		}
		gain = es7210_pga_code(value.vol * 2);
		if ((gain < 0) || ((config->vdda_mv == 1800U) && (value.vol < 21))) {
			return -EINVAL;
		}
	}
	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->configured) {
		ret = -EIO;
		goto out;
	}
	mask = es7210_channel_mask(dev, channel);
	if (mask < 0) {
		ret = mask;
		goto out;
	}
	if (property == AUDIO_PROPERTY_INPUT_MUTE) {
		uint8_t mute = value.mute ? (data->mute | mask) : (data->mute & ~mask);
		uint8_t effective = data->running ? (mute | ~data->processing.mic_mask) : 0x0fU;
		const struct es7210_reg regs[] = {
			{ES7210_MUTE12, effective & 3U},
			{ES7210_MUTE34, (effective >> 2) & 3U},
		};

		ret = es7210_write_sequence(dev, regs, ARRAY_SIZE(regs));
		if (ret == 0) {
			data->mute = mute;
		}
	} else {
		for (uint8_t i = 0U; i < 4U; i++) {
			if ((mask & BIT(i)) == 0U) {
				continue;
			}
			uint8_t select =
				es7210_microphone_gain(&data->processing, i) & ES7210_MIC_SELECT;

			ret = es7210_write(dev, ES7210_MIC1_GAIN + i, select | gain);
			if (ret != 0) {
				break;
			}
			data->processing.channel[i].pga_gain = value.vol * 2;
		}
	}
	if (ret != 0) {
		es7210_fault(dev);
	}
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int es7210_apply_properties(const struct device *dev)
{
	struct es7210_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = data->configured ? 0 : -EIO;
	k_mutex_unlock(&data->lock);
	return ret;
}

/* Legacy output-only entry points have no effect on a capture-only device. */
static void es7210_output(const struct device *dev)
{
	ARG_UNUSED(dev);
}

static int es7210_init(const struct device *dev)
{
	const struct es7210_config *config = dev->config;
	struct es7210_data *data = dev->data;

	k_mutex_init(&data->lock);
	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}
	return es7210_quiesce(dev);
}

static DEVICE_API(audio_codec, es7210_api) = {
	.configure = es7210_configure,
	.start_output = es7210_output,
	.stop_output = es7210_output,
	.set_property = es7210_set_property,
	.apply_properties = es7210_apply_properties,
	.start = es7210_start,
	.stop = es7210_stop,
};

static bool es7210_device_valid(const struct device *dev)
{
	return (dev != NULL) && (dev->api == &es7210_api) && device_is_ready(dev);
}

int audio_codec_es7210_configure(const struct device *dev,
				 const struct audio_codec_es7210_dai_cfg *cfg)
{
	if (!es7210_device_valid(dev)) {
		return -EINVAL;
	}
	return es7210_configure_serial(dev, cfg);
}

int audio_codec_es7210_get_processing(const struct device *dev,
				      struct audio_codec_es7210_processing *cfg)
{
	struct es7210_data *data;
	int ret = 0;

	if (!es7210_device_valid(dev) || (cfg == NULL)) {
		return -EINVAL;
	}
	data = dev->data;
	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->configured) {
		ret = -EIO;
	} else {
		*cfg = data->processing;
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

int audio_codec_es7210_set_processing(const struct device *dev,
				      const struct audio_codec_es7210_processing *cfg)
{
	struct es7210_data *data;
	int ret;

	if (!es7210_device_valid(dev) || (cfg == NULL)) {
		return -EINVAL;
	}
	data = dev->data;
	k_mutex_lock(&data->lock, K_FOREVER);
	if (!data->configured) {
		ret = -EIO;
		goto out;
	}
	if (data->running) {
		ret = -EBUSY;
		goto out;
	}
	ret = es7210_processing_validate(dev, cfg);
	if (ret != 0) {
		goto out;
	}
	ret = es7210_write_processing(dev, cfg);
	if (ret == 0) {
		data->processing = *cfg;
	} else {
		es7210_fault(dev);
	}
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

int audio_codec_es7210_get_status(const struct device *dev,
				  struct audio_codec_es7210_status *status)
{
	const struct es7210_config *config;
	struct es7210_data *data;
	uint8_t revision;
	uint8_t state;
	int ret;

	if (!es7210_device_valid(dev) || (status == NULL)) {
		return -EINVAL;
	}
	config = dev->config;
	data = dev->data;
	k_mutex_lock(&data->lock, K_FOREVER);
	ret = i2c_reg_read_byte_dt(&config->i2c, ES7210_REVISION, &revision);
	if (ret == 0) {
		ret = i2c_reg_read_byte_dt(&config->i2c, ES7210_STATUS, &state);
	}
	if (ret == 0) {
		*status = (struct audio_codec_es7210_status){
			.revision = revision,
			.state = state & 3U,
			.configured = data->configured,
			.running = data->running,
		};
	}
	k_mutex_unlock(&data->lock);
	return ret;
}

#define ES7210_DEFINE(inst)                                                                       \
	static struct es7210_data es7210_data_##inst;                                             \
	static const struct es7210_config es7210_config_##inst = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                \
		.sdout_pair = DT_INST_PROP(inst, everest_sdout) - 1U,                             \
		.bias = {                                                                         \
			DT_INST_ENUM_IDX(inst, everest_micbias_millivolt),                        \
			DT_INST_ENUM_IDX_OR(inst, everest_micbias34_millivolt,                    \
					    DT_INST_ENUM_IDX(inst, everest_micbias_millivolt)),   \
		},                                                                                \
		.vdda_mv = DT_INST_PROP(inst, everest_vdda_millivolt),                            \
		.vddd_mv = DT_INST_PROP(inst, everest_vddd_millivolt),                            \
	};                                                                                        \
	DEVICE_DT_INST_DEFINE(inst, es7210_init, NULL, &es7210_data_##inst, &es7210_config_##inst, \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY, &es7210_api);

DT_INST_FOREACH_STATUS_OKAY(ES7210_DEFINE)
