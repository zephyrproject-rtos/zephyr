/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_out.c
 * @brief CS47L63 output mixer, headphone amplifier, volume and mute
 *
 * The enable path follows the Apache-2.0 vendor sources rather than the field
 * names: the amplifier is brought up by setting OUT1L_EN and then waiting for
 * OUT1L_EN_STS, because the analog stage takes time the register write does not
 * account for, and driving audio into it before it reports ready is not visible
 * from any symbol in the map.
 *
 * The two halves of that are split across @ref cs47l63_out_start and
 * @ref cs47l63_out_confirm_start, because the analog stage cannot report ready
 * until SYSCLK runs and SYSCLK does not run until the caller has started the
 * I2S transfer that feeds FLL1 - which happens after start_output returns. The
 * level is put on the pins by the confirming half, so it is never applied to a
 * stage that never came up.
 */

#include "cs47l63_out.h"

#include <errno.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include "cs47l63_bus.h"
#include "cs47l63_clock.h"
#include "cs47l63_priv.h"
#include "cs47l63_regs.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(cs47l63);

/** Amplifier enable/disable poll: read every 2 ms, give up after 50 reads. */
#define CS47L63_OUT_POLL_MS  2
#define CS47L63_OUT_POLL_MAX 50

/** The volume field is 0.5 dB per step, so one dB is two codes. */
#define CS47L63_VOL_CODES_PER_DB 2

/**
 * @brief Convert a dB level to an OUT1L_VOL code, clamped to the envelope.
 */
static uint8_t vol_db_to_code(int vol_db)
{
	int db = CLAMP(vol_db, CS47L63_VOLUME_MIN_DB, CS47L63_VOLUME_MAX_DB);

	return (uint8_t)(CS47L63_OUT1L_VOL_0DB + (db * CS47L63_VOL_CODES_PER_DB));
}

/**
 * @brief Write OUT1L_VOLUME_1 with the update strobe set.
 *
 * OUT_VU is set on every write without exception: it is what latches the level
 * and the mute bit together, so a write without it leaves the field changed and
 * the output still at the previous value.
 */
static int write_volume_reg(const struct device *dev, uint8_t code, bool mute)
{
	uint32_t val = CS47L63_OUT_VU | code;

	if (mute) {
		val |= CS47L63_OUT1L_MUTE;
	}

	return cs47l63_bus_write_reg(dev, CS47L63_OUT1L_VOLUME_1, val);
}

/**
 * @brief Put the cached level on the pins if it belongs there.
 *
 * While the output is stopped the cache is updated and nothing is written, so
 * a level set before playback holds until the start applies it, and a stopped
 * output is never brought up to a level by a bare property set.
 */
static int apply_cached_volume(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;

	if (!data->output_running) {
		return 0;
	}

	return write_volume_reg(dev, data->vol_code, data->output_muted);
}

int cs47l63_out_init(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	data->output_running = false;
	data->output_muted = false;
	data->vol_code = CS47L63_OUT1L_VOL_0DB;
	data->src1 = CS47L63_MIXER_SRC_ASP1RX1;
	data->src2 = CS47L63_MIXER_SRC_ASP1RX2;

	/* Boot silent and unpowered: the amplifier off, the volume field at its
	 * cached level but muted. Nothing reaches the jack until start_output.
	 */
	ret = cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0);
	if (ret < 0) {
		return ret;
	}

	return write_volume_reg(dev, data->vol_code, true);
}

/** @brief Write one mixer input slot: a source and its 0 dB mix volume. */
static int write_mixer_slot(const struct device *dev, uint32_t addr, uint16_t src)
{
	return cs47l63_bus_update_reg(
		dev, addr, CS47L63_OUT1LMIX_VOL_MASK | CS47L63_OUT1L_SRC_MASK,
		((uint32_t)CS47L63_OUT1LMIX_VOL_0DB << CS47L63_OUT1LMIX_VOL_SHIFT) | src);
}

static int write_route(const struct device *dev)
{
	const struct cs47l63_data *data = dev->data;
	int ret;

	ret = write_mixer_slot(dev, CS47L63_OUT1L_INPUT1, data->src1);
	if (ret < 0) {
		return ret;
	}

	return write_mixer_slot(dev, CS47L63_OUT1L_INPUT2, data->src2);
}

int cs47l63_out_route_output(const struct device *dev, audio_channel_t channel, uint32_t output)
{
	struct cs47l63_data *data = dev->data;
	uint16_t src1;
	uint16_t src2;

	if ((enum cs47l63_output)output != CS47L63_OUTPUT_HP) {
		LOG_ERR("This part has only the headphone terminal");
		return -ENOTSUP;
	}

	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
	case AUDIO_CHANNEL_HEADPHONE_LEFT:
		src1 = CS47L63_MIXER_SRC_ASP1RX1;
		src2 = CS47L63_MIXER_SRC_NONE;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
	case AUDIO_CHANNEL_HEADPHONE_RIGHT:
		src1 = CS47L63_MIXER_SRC_ASP1RX2;
		src2 = CS47L63_MIXER_SRC_NONE;
		break;
	case AUDIO_CHANNEL_ALL:
		/* One physical channel, two slots: both receive slots are mixed
		 * into it.
		 */
		src1 = CS47L63_MIXER_SRC_ASP1RX1;
		src2 = CS47L63_MIXER_SRC_ASP1RX2;
		break;
	default:
		return -ENOTSUP;
	}

	data->src1 = src1;
	data->src2 = src2;

	return write_route(dev);
}

int cs47l63_out_start(const struct device *dev)
{
	int ret;

	/* Re-arm at the route and level already chosen rather than redoing
	 * configure()'s work.
	 */
	ret = write_route(dev);
	if (ret < 0) {
		return ret;
	}

	/* The enable is written here and confirmed later. The analog stage
	 * needs SYSCLK to come up at all, SYSCLK needs FLL1 locked, and FLL1's
	 * reference is the SoC's I2S master clock - which the caller starts
	 * after it has told this codec to play. Waiting for OUT1L_EN_STS at
	 * this point therefore cannot succeed on a cold boot, and the wait
	 * belongs where the clock exists: cs47l63_out_confirm_start().
	 */
	return cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN,
				      CS47L63_OUT1L_EN);
}

int cs47l63_out_confirm_start(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	int ret;

	ret = cs47l63_bus_poll_reg(dev, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS,
				   CS47L63_OUT1L_EN_STS, CS47L63_OUT_POLL_MS, CS47L63_OUT_POLL_MAX);
	if (ret < 0) {
		LOG_ERR("Headphone amplifier never reported enabled");
		return ret;
	}

	/* Only now is the output live, so only now may the cached level go on
	 * the pins: this is the point the synchronous wait used to reach.
	 */
	data->output_running = true;

	return apply_cached_volume(dev);
}

int cs47l63_out_stop(const struct device *dev)
{
	struct cs47l63_data *data = dev->data;
	bool locked;
	int ret;

	/* Mute before the amplifier goes down, so the stage is silent while it
	 * collapses rather than after.
	 */
	ret = write_volume_reg(dev, data->vol_code, true);
	if (ret < 0) {
		return ret;
	}

	data->output_running = false;

	ret = cs47l63_bus_update_reg(dev, CS47L63_OUTPUT_ENABLE_1, CS47L63_OUT1L_EN, 0);
	if (ret < 0) {
		return ret;
	}

	/* The status bit is updated by the part's own clock domain, and by the
	 * time a route tears down, the I2S transfer that gives FLL1 its
	 * reference has usually already stopped. The amplifier does go down -
	 * the enable bit above is what takes it down - but nothing is left
	 * running to report that it did, so waiting can only ever time out.
	 * Wait while there is a clock to be waited on, and take the write's
	 * word for it when there is not.
	 */
	ret = cs47l63_clock_locked(dev, &locked);
	if (ret < 0) {
		return ret;
	}

	if (!locked) {
		return 0;
	}

	ret = cs47l63_bus_poll_reg(dev, CS47L63_OUTPUT_STATUS_1, CS47L63_OUT1L_EN_STS, 0,
				   CS47L63_OUT_POLL_MS, CS47L63_OUT_POLL_MAX);
	if (ret < 0) {
		LOG_ERR("Headphone amplifier never reported disabled");
	}

	return ret;
}

int cs47l63_out_set_volume(const struct device *dev, int vol)
{
	struct cs47l63_data *data = dev->data;

	data->vol_code = vol_db_to_code(vol);

	return apply_cached_volume(dev);
}

int cs47l63_out_set_mute(const struct device *dev, bool mute)
{
	struct cs47l63_data *data = dev->data;

	data->output_muted = mute;

	return apply_cached_volume(dev);
}
