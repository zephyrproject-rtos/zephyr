/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_out.h
 * @brief CS47L63 output mixer, headphone amplifier, volume and mute (internal)
 *
 * This part has exactly one output channel, OUT1L, driving a differential
 * headphone load. There is no OUT1R and no OUT2 anywhere in its register map,
 * so "left" and "right" here select which serial-port slot feeds that one
 * channel, not which of two amplifiers is addressed.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_OUT_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_OUT_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Physical output terminal selected by @ref cs47l63_out_route_output. */
enum cs47l63_output {
	CS47L63_OUTPUT_HP = 0,   /**< OUTP/OUTN, the differential headphone driver. */
	CS47L63_OUTPUT_LINE = 1, /**< Not present on this part. */
};

/** @brief Volume envelope, in dB, of the OUT1L digital volume control. */
#define CS47L63_VOLUME_MIN_DB (-64)
#define CS47L63_VOLUME_MAX_DB 0

/**
 * @brief Put the output stage into its boot state: muted, routed to nothing.
 *
 * Called from configure() after the clock and the serial port are up. Leaves
 * the amplifier disabled - configure() does not start audio.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_out_init(const struct device *dev);

/**
 * @brief Class API @c start_output: route the mixer and enable the amplifier.
 *
 * Does not repeat configure()'s work: it re-arms the output at the route
 * already chosen. Does not wait for the amplifier and does not put the level
 * on the pins - the amplifier cannot report enabled until SYSCLK runs, and
 * SYSCLK does not run until the caller starts the I2S transfer that gives FLL1
 * its reference, which happens after this returns. The start is completed by
 * @ref cs47l63_out_confirm_start; until then the output is enabled but still
 * muted and @c output_running is false.
 *
 * @param dev Codec device
 * @return 0 on success
 * @retval negative errno from the failing bus transaction
 */
int cs47l63_out_start(const struct device *dev);

/**
 * @brief Complete a start once the clock that the amplifier needs exists.
 *
 * The second half of @ref cs47l63_out_start, called from the deferred check
 * that start_output arms. Waits for OUT1L_EN_STS and, only if it appears,
 * marks the output running and puts the cached level on the pins - leaving the
 * driver in exactly the state a start that could have waited would have.
 *
 * A failure here is a real one: the enable was written, the clock has had time
 * to arrive, and the stage still does not report ready.
 *
 * @param dev Codec device
 * @return 0 on success
 * @retval -ETIMEDOUT if the amplifier never reports enabled
 * @retval other negative errno from the failing bus transaction
 */
int cs47l63_out_confirm_start(const struct device *dev);

/**
 * @brief Class API @c stop_output: mute, then bring the amplifier down.
 *
 * Waits for the shutdown to complete for the same reason the startup is
 * waited for: the next thing a caller does may be to cut the clock.
 *
 * @param dev Codec device
 * @return 0 on success, -ETIMEDOUT if the amplifier never reports disabled,
 *         other negative errno from the failing bus transaction
 */
int cs47l63_out_stop(const struct device *dev);

/**
 * @brief Apply an @ref AUDIO_PROPERTY_OUTPUT_VOLUME request in dB.
 *
 * A value outside @ref CS47L63_VOLUME_MIN_DB to @ref CS47L63_VOLUME_MAX_DB is
 * clamped to the envelope, never wrapped. A volume set while the output is
 * stopped is remembered and applied when it starts, so a caller that sets the
 * level before starting audio gets that level.
 *
 * @param dev Codec device
 * @param vol Requested volume in dB
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_out_set_volume(const struct device *dev, int vol);

/**
 * @brief Class API @c set_property @c AUDIO_PROPERTY_OUTPUT_MUTE.
 *
 * The mute state is held apart from the volume cache, so unmuting returns to
 * the level that was last set - before the mute or during it - rather than to
 * a default or the reset value.
 *
 * @param dev  Codec device
 * @param mute true to silence the output, false to restore the cached level
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_out_set_mute(const struct device *dev, bool mute);

/**
 * @brief Class API @c route_output: feed the headphone channel from ASP1.
 *
 * @param dev     Codec device
 * @param channel AUDIO_CHANNEL_ALL (both receive slots mixed into the one
 *                output channel), AUDIO_CHANNEL_FRONT_LEFT or
 *                AUDIO_CHANNEL_HEADPHONE_LEFT (slot 1 alone),
 *                AUDIO_CHANNEL_FRONT_RIGHT or AUDIO_CHANNEL_HEADPHONE_RIGHT
 *                (slot 2 alone); any other value is rejected
 * @param output  A @ref cs47l63_output value. Only the headphone terminal
 *                exists on this part; the line terminal is refused rather than
 *                quietly routed to the headphone
 * @return 0 on success, -ENOTSUP for an unsupported channel or terminal
 *         (nothing written), negative errno on a bus failure
 */
int cs47l63_out_route_output(const struct device *dev, audio_channel_t channel, uint32_t output);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_OUT_H_ */
