/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>

#include "tlv320aic3104_bus.h"
#include "tlv320aic3104_in.h"
#include "tlv320aic3104_priv.h"
#include "tlv320aic3104_regs.h"

int tlv320aic3104_in_set_line2_routing(const struct device *dev, bool is_mono)
{
	int ret;

	ret = tlv320aic3104_bus_write_reg(dev, 0, MIC2_LINE2_TO_LADC,
					  is_mono ? LINE2_TO_ADC_MONO : LINE2_TO_ADC_STEREO_L);
	if (ret < 0) {
		return ret;
	}

	return tlv320aic3104_bus_write_reg(dev, 0, MIC2_LINE2_TO_RADC,
					   is_mono ? LINE2_TO_ADC_MONO : LINE2_TO_ADC_STEREO_R);
}

static int set_adc_power(const struct device *dev, uint8_t addr, bool on)
{
	uint8_t val;
	int ret;

	ret = tlv320aic3104_bus_read_reg(dev, 0, addr, &val);
	if (ret < 0) {
		return ret;
	}

	if (on) {
		val |= ADC_CHANNEL_POWER_BIT;
	} else {
		val &= (uint8_t)~ADC_CHANNEL_POWER_BIT;
	}

	return tlv320aic3104_bus_write_reg(dev, 0, addr, val);
}

static int set_line1_field(const struct device *dev, uint8_t addr, bool connect)
{
	uint8_t val;
	int ret;

	ret = tlv320aic3104_bus_read_reg(dev, 0, addr, &val);
	if (ret < 0) {
		return ret;
	}

	val &= LINE1_TO_ADC_FIELD_PRESERVE_MASK;
	val |= connect ? LINE1_TO_ADC_CONNECT_0DB : LINE1_TO_ADC_DISCONNECT;

	return tlv320aic3104_bus_write_reg(dev, 0, addr, val);
}

static int route_left_adc(const struct device *dev, enum tlv320aic3104_input input)
{
	int ret;

	if (input == TLV320AIC3104_INPUT_LINE2) {
		ret = tlv320aic3104_bus_write_reg(dev, 0, MIC2_LINE2_TO_LADC,
						  LINE2_TO_ADC_STEREO_L);
		if (ret < 0) {
			return ret;
		}
		return set_line1_field(dev, LINE1L_TO_LADC_CTRL, false);
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, MIC2_LINE2_TO_LADC, LINE2_TO_ADC_DISCONNECT);
	if (ret < 0) {
		return ret;
	}
	return set_line1_field(dev, LINE1L_TO_LADC_CTRL, true);
}

static int route_right_adc(const struct device *dev, enum tlv320aic3104_input input)
{
	int ret;

	if (input == TLV320AIC3104_INPUT_LINE2) {
		ret = tlv320aic3104_bus_write_reg(dev, 0, MIC2_LINE2_TO_RADC,
						  LINE2_TO_ADC_STEREO_R);
		if (ret < 0) {
			return ret;
		}
		return set_line1_field(dev, LINE1R_TO_RADC_CTRL, false);
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, MIC2_LINE2_TO_RADC, LINE2_TO_ADC_DISCONNECT);
	if (ret < 0) {
		return ret;
	}
	return set_line1_field(dev, LINE1R_TO_RADC_CTRL, true);
}

int tlv320aic3104_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	bool do_left;
	bool do_right;
	int ret;

	if (input != TLV320AIC3104_INPUT_LINE1 && input != TLV320AIC3104_INPUT_LINE2) {
		return -ENOTSUP;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		do_left = true;
		do_right = false;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		do_left = false;
		do_right = true;
		break;
	case AUDIO_CHANNEL_ALL:
		do_left = true;
		do_right = true;
		break;
	default:
		return -ENOTSUP;
	}

	if (do_left) {
		ret = route_left_adc(dev, (enum tlv320aic3104_input)input);
		if (ret < 0) {
			return ret;
		}
	}
	if (do_right) {
		ret = route_right_adc(dev, (enum tlv320aic3104_input)input);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define ADC_PGA_GAIN_MAX_DIDB (ADC_PGA_GAIN_MAX_CODE * 5)

static uint8_t pga_gain_code_from_didb(int gain_didb)
{
	if (gain_didb < 0) {
		gain_didb = 0;
	} else if (gain_didb > ADC_PGA_GAIN_MAX_DIDB) {
		gain_didb = ADC_PGA_GAIN_MAX_DIDB;
	}

	return (uint8_t)(gain_didb / 5);
}

int tlv320aic3104_in_set_pga_gain(const struct device *dev, audio_channel_t channel, int gain_didb)
{
	struct tlv320aic3104_data *data = dev->data;
	const uint8_t code = pga_gain_code_from_didb(gain_didb);
	int ret;

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		data->last_left_adc_pga = code;
		return tlv320aic3104_bus_write_reg(dev, 0, LEFT_ADC_PGA_GAIN, code);
	case AUDIO_CHANNEL_FRONT_RIGHT:
		data->last_right_adc_pga = code;
		return tlv320aic3104_bus_write_reg(dev, 0, RIGHT_ADC_PGA_GAIN, code);
	case AUDIO_CHANNEL_ALL:
		data->last_left_adc_pga = code;
		data->last_right_adc_pga = code;
		ret = tlv320aic3104_bus_write_reg(dev, 0, LEFT_ADC_PGA_GAIN, code);
		if (ret < 0) {
			return ret;
		}
		return tlv320aic3104_bus_write_reg(dev, 0, RIGHT_ADC_PGA_GAIN, code);
	default:
		return -ENOTSUP;
	}
}

static uint8_t micbias_reg_value(uint8_t level)
{
	switch (level) {
	case 1:
		return MICBIAS_2V;
	case 2:
		return MICBIAS_2V5;
	case 3:
		return MICBIAS_AVDD;
	default:
		return MICBIAS_OFF;
	}
}

static uint8_t adc_hpf_reg_value(uint8_t code)
{
	const uint8_t nibble = code & 0x03U;

	return (uint8_t)((nibble << 6) | (nibble << 4));
}

int tlv320aic3104_in_start(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	const struct tlv320aic3104_data *data = dev->data;
	const bool is_mono = (data->channel_mode == TLV320AIC3104_CHANNEL_MONO);
	int ret;

	ret = tlv320aic3104_in_set_line2_routing(dev, is_mono);
	if (ret < 0) {
		return ret;
	}

	ret = set_adc_power(dev, LINE1L_TO_LADC_CTRL, true);
	if (ret < 0) {
		return ret;
	}
	ret = set_adc_power(dev, LINE1R_TO_RADC_CTRL, true);
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, MICBIAS_CTRL,
					  micbias_reg_value(cfg->mic_bias_level));
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, CODEC_FILTER, adc_hpf_reg_value(cfg->adc_hpf));
	if (ret < 0) {
		return ret;
	}

	ret = tlv320aic3104_bus_write_reg(dev, 0, LEFT_ADC_PGA_GAIN, data->last_left_adc_pga);
	if (ret < 0) {
		return ret;
	}
	return tlv320aic3104_bus_write_reg(dev, 0, RIGHT_ADC_PGA_GAIN, data->last_right_adc_pga);
}

int tlv320aic3104_in_stop(const struct device *dev)
{
	int ret;

	ret = tlv320aic3104_bus_write_reg(dev, 0, LEFT_ADC_PGA_GAIN, ADC_PGA_MUTE);
	if (ret < 0) {
		return ret;
	}
	ret = tlv320aic3104_bus_write_reg(dev, 0, RIGHT_ADC_PGA_GAIN, ADC_PGA_MUTE);
	if (ret < 0) {
		return ret;
	}

	ret = set_adc_power(dev, LINE1L_TO_LADC_CTRL, false);
	if (ret < 0) {
		return ret;
	}
	return set_adc_power(dev, LINE1R_TO_RADC_CTRL, false);
}
