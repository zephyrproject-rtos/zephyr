/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include "tlv320aic3104_bus.h"
#include "tlv320aic3104_fault.h"
#include "tlv320aic3104_out.h"
#include "tlv320aic3104_priv.h"
#include "tlv320aic3104_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tlv320aic3104);

static void write_output_volume_regs(const struct device *dev, uint8_t dac_vol,
				     uint8_t routing_vol);

static uint8_t left_mixer_reg(uint8_t output)
{
	return (output == TLV320AIC3104_OUTPUT_LOP) ? DAC_L1_TO_LEFT_LOP_VOL : DAC_L1_TO_HPLOUT_VOL;
}

static uint8_t right_mixer_reg(uint8_t output)
{
	return (output == TLV320AIC3104_OUTPUT_LOP) ? DAC_R1_TO_RIGHT_LOP_VOL
						    : DAC_R1_TO_HPROUT_VOL;
}

int tlv320aic3104_out_init_hprcom(const struct device *dev)
{
	const struct tlv320aic3104_config *cfg = dev->config;
	int ret;

	if (!cfg->hprcom_vcm_output) {
		return 0;
	}

	ret = tlv320aic3104_bus_update_reg(dev, 0, HP_OUTPUT_DRIVER_CTRL,
					   HP_OUTPUT_DRIVER_HPRCOM_MODE_MASK,
					   HP_OUTPUT_DRIVER_HPRCOM_MODE_VCM);
	if (ret < 0) {
		return ret;
	}

	return tlv320aic3104_bus_write_reg(dev, 0, HPRCOM_LEVEL, HPRCOM_LEVEL_0DB_UNMUTED_POWERED);
}

void tlv320aic3104_out_start(const struct device *dev)
{
	struct tlv320aic3104_data *data = dev->data;

	data->output_running = true;

	if (data->output_muted) {
		write_output_volume_regs(dev, DAC_VOL_MUTE, DAC_TO_OUT_ROUTED_MUTE);
	} else {
		write_output_volume_regs(dev, data->last_dac_vol, data->last_routing_vol);
	}

	(void)tlv320aic3104_fault_check(dev);
}

void tlv320aic3104_out_stop(const struct device *dev)
{
	struct tlv320aic3104_data *data = dev->data;

	data->output_running = false;

	tlv320aic3104_bus_write_reg(dev, 0, DAC_L1_TO_HPLOUT_VOL, DAC_TO_OUT_ROUTED_MUTE);
	tlv320aic3104_bus_write_reg(dev, 0, DAC_R1_TO_HPROUT_VOL, DAC_TO_OUT_ROUTED_MUTE);

	tlv320aic3104_bus_write_reg(dev, 0, DAC_L1_TO_LEFT_LOP_VOL, DAC_TO_OUT_ROUTED_MUTE);
	tlv320aic3104_bus_write_reg(dev, 0, DAC_R1_TO_RIGHT_LOP_VOL, DAC_TO_OUT_ROUTED_MUTE);
	tlv320aic3104_bus_write_reg(dev, 0, LEFT_DAC_VOL, DAC_VOL_MUTE);
	tlv320aic3104_bus_write_reg(dev, 0, RIGHT_DAC_VOL, DAC_VOL_MUTE);
}

static void write_output_volume_regs(const struct device *dev, uint8_t dac_vol, uint8_t routing_vol)
{
	const struct tlv320aic3104_data *data = dev->data;

	tlv320aic3104_bus_write_reg(dev, 0, LEFT_DAC_VOL, dac_vol);
	tlv320aic3104_bus_write_reg(dev, 0, RIGHT_DAC_VOL, dac_vol);
	tlv320aic3104_bus_write_reg(dev, 0, left_mixer_reg(data->output_left), routing_vol);
	tlv320aic3104_bus_write_reg(dev, 0, right_mixer_reg(data->output_right), routing_vol);
}

static void apply_output_volume_regs(const struct device *dev, uint8_t dac_vol,
					uint8_t routing_vol)
{
	struct tlv320aic3104_data *data = dev->data;

	data->last_dac_vol = dac_vol;
	data->last_routing_vol = routing_vol;

	if (!data->output_running || data->output_muted) {
		return;
	}

	write_output_volume_regs(dev, dac_vol, routing_vol);
}

int tlv320aic3104_out_set_volume(const struct device *dev, int vol)
{
	uint8_t dac_vol;
	uint8_t routing_vol;
	int db;

	if (vol > 0 || vol < -CODEC_VOLUME_ATTEN_MAX) {
		return -EINVAL;
	}

	db = CLAMP(-vol, 0, CODEC_VOLUME_ATTEN_MAX);

	dac_vol = (uint8_t)db;
	routing_vol = DAC_TO_OUT_ROUTED_0DB | (uint8_t)db;

	apply_output_volume_regs(dev, dac_vol, routing_vol);

	return 0;
}

void tlv320aic3104_out_set_mute(const struct device *dev, bool mute)
{
	struct tlv320aic3104_data *data = dev->data;

	data->output_muted = mute;

	if (!data->output_running) {
		return;
	}

	if (mute) {
		write_output_volume_regs(dev, DAC_VOL_MUTE, DAC_TO_OUT_ROUTED_MUTE);
	} else {
		write_output_volume_regs(dev, data->last_dac_vol, data->last_routing_vol);
	}
}

static int set_route_bit(const struct device *dev, uint8_t addr, bool routed)
{
	return tlv320aic3104_bus_update_reg(dev, 0, addr, DAC_TO_OUT_ROUTE_BIT,
					    routed ? DAC_TO_OUT_ROUTE_BIT : 0);
}

static int select_terminal(const struct device *dev, uint8_t selected, uint8_t other)
{
	int ret = set_route_bit(dev, selected, true);

	if (ret < 0) {
		return ret;
	}
	return set_route_bit(dev, other, false);
}

int tlv320aic3104_out_route_output(const struct device *dev, audio_channel_t channel,
				   uint32_t output)
{
	struct tlv320aic3104_data *data = dev->data;
	uint8_t selected_left;
	uint8_t other_left;
	uint8_t selected_right;
	uint8_t other_right;
	bool do_left;
	bool do_right;
	int ret;

	switch ((enum tlv320aic3104_output)output) {
	case TLV320AIC3104_OUTPUT_HP:
		selected_left = DAC_L1_TO_HPLOUT_VOL;
		other_left = DAC_L1_TO_LEFT_LOP_VOL;
		selected_right = DAC_R1_TO_HPROUT_VOL;
		other_right = DAC_R1_TO_RIGHT_LOP_VOL;
		break;
	case TLV320AIC3104_OUTPUT_LOP:
		selected_left = DAC_L1_TO_LEFT_LOP_VOL;
		other_left = DAC_L1_TO_HPLOUT_VOL;
		selected_right = DAC_R1_TO_RIGHT_LOP_VOL;
		other_right = DAC_R1_TO_HPROUT_VOL;
		break;
	default:
		return -ENOTSUP;
	}

	ret = tlv320aic3104_channel_to_lr(channel, &do_left, &do_right);
	if (ret < 0) {
		return ret;
	}

	if (do_left) {
		ret = select_terminal(dev, selected_left, other_left);
		if (ret < 0) {
			return ret;
		}
		data->output_left = (uint8_t)output;
	}
	if (do_right) {
		ret = select_terminal(dev, selected_right, other_right);
		if (ret < 0) {
			return ret;
		}
		data->output_right = (uint8_t)output;
	}

	return 0;
}

int tlv320aic3104_out_set_channel_mode(const struct device *dev, bool is_mono)
{
	return tlv320aic3104_bus_update_reg(dev, 0, CODEC_DATAPATH_SETUP,
					    CODEC_DATAPATH_MODE_FIELD_MASK,
					    is_mono ? CODEC_DATAPATH_DAC_MONO
						    : CODEC_DATAPATH_DAC_STEREO);
}
