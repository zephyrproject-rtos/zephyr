/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_in.c
 * @brief CS47L63 analog line input and the ASP1 transmit mixers
 *
 * Field layouts come from the Apache-2.0 vendor register map
 * (modules/hal/cirrus-logic/cs47l63/cs47l63_spec.h); the two transmit mixers
 * are laid out identically to the OUT1L mixer slots in cs47l63_out.c, which is
 * why the slot helper here is the same shape as the one there rather than a
 * shared one - the masks belong to different registers and pretending they are
 * one control is how a change to the output path silently reaches the input.
 */

#include "cs47l63_in.h"

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include "cs47l63_bus.h"
#include "cs47l63_priv.h"
#include "cs47l63_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(cs47l63);

/** Both halves of the line pair, as one enable mask. */
#define CS47L63_IN2_EN_BOTH (CS47L63_IN2L_EN | CS47L63_IN2R_EN)

/** Both ASP1 transmit slots, as one enable mask. */
#define CS47L63_ASP1_TX_EN_BOTH (CS47L63_ASP1_TX1_EN | CS47L63_ASP1_TX2_EN)

/**
 * @brief Write one transmit mixer slot: a source and its 0 dB mix volume.
 */
static int write_tx_mixer_slot(const struct device *dev, uint32_t addr, uint16_t src)
{
	return cs47l63_bus_update_reg(
		dev, addr, CS47L63_OUT1LMIX_VOL_MASK | CS47L63_OUT1L_SRC_MASK,
		((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | src);
}

/**
 * @brief Point both transmit mixers at the cached sources.
 */
static int write_route(const struct device *dev)
{
	const struct cs47l63_data *data = dev->data;
	int ret;

	ret = write_tx_mixer_slot(dev, CS47L63_ASP1TX1_INPUT1, data->in_src1);
	if (ret < 0) {
		return ret;
	}

	return write_tx_mixer_slot(dev, CS47L63_ASP1TX2_INPUT1, data->in_src2);
}

/**
 * @brief Set both halves of the line pair muted or unmuted at 0 dB.
 *
 * IN_VU is strobed afterwards without exception: it is what latches the two
 * volume fields, so a write without it leaves them changed and the front end
 * still at the previous level.
 */
static int write_input_level(const struct device *dev, bool mute)
{
	static const uint32_t k_control2[] = {CS47L63_IN2L_CONTROL2, CS47L63_IN2R_CONTROL2};
	uint32_t val = ((uint32_t)CS47L63_IN2_VOL_0DB << CS47L63_IN2_VOL_SHIFT) |
		       ((uint32_t)CS47L63_IN2_PGA_VOL_0DB << CS47L63_IN2_PGA_VOL_SHIFT);
	int ret;

	if (mute) {
		val |= CS47L63_IN2_MUTE;
	}

	for (size_t i = 0; i < ARRAY_SIZE(k_control2); i++) {
		ret = cs47l63_bus_update_reg(
			dev, k_control2[i],
			CS47L63_IN2_MUTE | CS47L63_IN2_VOL_MASK | CS47L63_IN2_PGA_VOL_MASK, val);
		if (ret < 0) {
			return ret;
		}
	}

	return cs47l63_bus_write_reg(dev, CS47L63_INPUT_CONTROL3, CS47L63_IN_VU);
}

int cs47l63_in_init(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->in_running = false;
	data->in_src1 = CS47L63_MIXER_SRC_IN2L;
	data->in_src2 = CS47L63_MIXER_SRC_IN2R;

	/* Boot quiet: the transmit slots off, the front end disabled, the two
	 * mixers pointed at nothing. Nothing reaches ASP1DOUT until a start.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_ENABLES1, CS47L63_ASP1_TX_EN_BOTH, 0);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL, CS47L63_IN2_EN_BOTH, 0);
	if (ret < 0) {
		return ret;
	}

	ret = write_tx_mixer_slot(dev, CS47L63_ASP1TX1_INPUT1, CS47L63_MIXER_SRC_NONE);
	if (ret < 0) {
		return ret;
	}

	ret = write_tx_mixer_slot(dev, CS47L63_ASP1TX2_INPUT1, CS47L63_MIXER_SRC_NONE);
	if (ret < 0) {
		return ret;
	}

	return write_input_level(dev, true);
}

int cs47l63_in_start(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	/* Analog, not PDM, at the oversampling the 48 kHz family runs. The rate
	 * field of IN2nCONTROL1 is left alone: its reset value already selects
	 * SAMPLE_RATE1, which is the slot cs47l63_dai.c programs.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT2_CONTROL1,
				     CS47L63_IN2_OSR_MASK | CS47L63_IN2_MODE,
				     (uint32_t)CS47L63_IN2_OSR_3M072 << CS47L63_IN2_OSR_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_IN2L_CONTROL1, CS47L63_IN2_SRC_MASK,
				     (uint32_t)CS47L63_IN2_SRC_SINGLE_ENDED
					     << CS47L63_IN2_SRC_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_IN2R_CONTROL1, CS47L63_IN2_SRC_MASK,
				     (uint32_t)CS47L63_IN2_SRC_SINGLE_ENDED
					     << CS47L63_IN2_SRC_SHIFT);
	if (ret < 0) {
		return ret;
	}

	ret = write_route(dev);
	if (ret < 0) {
		return ret;
	}

	ret = write_input_level(dev, false);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL, CS47L63_IN2_EN_BOTH,
				     CS47L63_IN2_EN_BOTH);
	if (ret < 0) {
		return ret;
	}

	/* An update, never a write: the receive slots cs47l63_dai.c enabled
	 * live in this same register, and a full write would take the playback
	 * path down with it.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_ENABLES1, CS47L63_ASP1_TX_EN_BOTH,
				     CS47L63_ASP1_TX_EN_BOTH);
	if (ret < 0) {
		return ret;
	}

	data->in_running = true;

	return 0;
}

int cs47l63_in_stop(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->in_running = false;

	ret = cs47l63_bus_update_reg(dev, CS47L63_ASP1_ENABLES1, CS47L63_ASP1_TX_EN_BOTH, 0);
	if (ret < 0) {
		return ret;
	}

	ret = write_input_level(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = cs47l63_bus_update_reg(dev, CS47L63_INPUT_CONTROL, CS47L63_IN2_EN_BOTH, 0);
	if (ret < 0) {
		return ret;
	}

	ret = write_tx_mixer_slot(dev, CS47L63_ASP1TX1_INPUT1, CS47L63_MIXER_SRC_NONE);
	if (ret < 0) {
		return ret;
	}

	return write_tx_mixer_slot(dev, CS47L63_ASP1TX2_INPUT1, CS47L63_MIXER_SRC_NONE);
}

int cs47l63_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input)
{
	struct cs47l63_data *data = dev->data;
	uint16_t src1;
	uint16_t src2;

	if ((enum cs47l63_input)input != CS47L63_INPUT_LINE) {
		LOG_ERR("Only the analog line pair is brought up by this driver");
		return -ENOTSUP;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		/* One pin into both slots, so a mono source still arrives on
		 * both halves of the stereo frame the serial port carries.
		 */
		src1 = CS47L63_MIXER_SRC_IN2L;
		src2 = CS47L63_MIXER_SRC_IN2L;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		src1 = CS47L63_MIXER_SRC_IN2R;
		src2 = CS47L63_MIXER_SRC_IN2R;
		break;
	case AUDIO_CHANNEL_ALL:
		src1 = CS47L63_MIXER_SRC_IN2L;
		src2 = CS47L63_MIXER_SRC_IN2R;
		break;
	default:
		return -ENOTSUP;
	}

	data->in_src1 = src1;
	data->in_src2 = src2;

	if (!data->in_running) {
		/* Remembered, not written: a route chosen while stopped is
		 * applied by the next start, like the output path does.
		 */
		return 0;
	}

	return write_route(dev);
}
