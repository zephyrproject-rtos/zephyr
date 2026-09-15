/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_in.h
 * @brief CS47L63 analog line input and the ASP1 transmit mixers (internal)
 *
 * The mirror image of cs47l63_out.h. Only the IN2 pair is brought up: it is
 * the one the boards wire to a connector. IN1 is the PDM microphone pair,
 * which needs a bias supply and a clock this driver never configures, so
 * asking for it is refused rather than silently served from IN2.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_IN_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_IN_H_

#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Physical input terminal selected by @ref cs47l63_in_route_input. */
enum cs47l63_input {
	CS47L63_INPUT_LINE = 0, /**< IN2L/IN2R, the analog line pair. */
	CS47L63_INPUT_PDM = 1,  /**< IN1L/IN1R; not brought up by this driver. */
};

/**
 * @brief Put the input stage into its boot state: disabled and muted.
 *
 * Called from configure() alongside @ref cs47l63_out_init, so a device that
 * only ever plays back still leaves the ADC pins quiet instead of at whatever
 * the reset defaults are.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_in_init(const struct device *dev);

/**
 * @brief Class API @c start on @c AUDIO_DAI_DIR_RX: enable the line input.
 *
 * Configures the analog front end, unmutes it at 0 dB, points the two ASP1
 * transmit mixers at the routed sources and enables the transmit slots.
 *
 * Deliberately does not wait for INPUT_STATUS to report the inputs up: the
 * caller starts the I2S transfer that gives FLL1 its reference only after this
 * returns, so at this point SYSCLK does not run and the status can never
 * appear. Waiting here is what @ref cs47l63_out_start had to be split apart to
 * avoid; the input stage needs no second half, because nothing about it has to
 * be applied once it is running.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_in_start(const struct device *dev);

/**
 * @brief Class API @c stop on @c AUDIO_DAI_DIR_RX: mute and disable it again.
 *
 * Leaves the part in the state @ref cs47l63_in_init produced, including the
 * transmit mixer sources, so a stop followed by a start does not depend on
 * whatever the previous route was.
 *
 * @param dev Codec device
 * @return 0 on success, negative errno on a bus failure
 */
int cs47l63_in_stop(const struct device *dev);

/**
 * @brief Class API @c route_input: pick which pins feed the transmit slots.
 *
 * Takes effect immediately when the input is running and is remembered for the
 * next start when it is not, matching @ref cs47l63_out_route_output.
 *
 * @param dev     Codec device
 * @param channel AUDIO_CHANNEL_ALL (IN2L to transmit slot 1, IN2R to slot 2),
 *                AUDIO_CHANNEL_FRONT_LEFT (IN2L into both slots) or
 *                AUDIO_CHANNEL_FRONT_RIGHT (IN2R into both slots); any other
 *                value is rejected
 * @param input   A @ref cs47l63_input value. Only the line pair is supported
 * @return 0 on success, -ENOTSUP for an unsupported channel or terminal
 *         (nothing written), negative errno on a bus failure
 */
int cs47l63_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_IN_H_ */
