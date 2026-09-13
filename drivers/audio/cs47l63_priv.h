/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_priv.h
 * @brief CS47L63 instance config/data structs, shared by the driver's
 *        translation units (internal)
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_PRIV_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_PRIV_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Per-instance devicetree-derived configuration. */
struct cs47l63_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec irq_gpio;
	/** Optional: only present on a board that wires the codec's GPIO9. */
	struct gpio_dt_spec gpio9_gpio;
};

/** Per-instance runtime state. */
struct cs47l63_data {
	/* Owned by cs47l63_out.c. vol_code is the OUT1L_VOL code last
	 * requested, held whether or not it is on the pins: output_running
	 * says whether the amplifier is up, output_muted whether the class
	 * API's mute property is holding it silent. Keeping the three apart is
	 * what lets a volume set while stopped survive to the next start, and a
	 * mute/unmute round trip give the level back instead of a default.
	 */
	uint8_t vol_code;
	bool output_running;
	bool output_muted;
	/* The two OUT1L mixer input slots, as mixer source codes. Re-written on
	 * every start_output so a route chosen while stopped takes effect.
	 */
	uint16_t src1;
	uint16_t src2;
	/* Owned by cs47l63_in.c, the same three-way split as the output side:
	 * in_running says whether the analog front end is enabled, in_src1 and
	 * in_src2 are the ASP1 transmit mixer sources, held across a stop so a
	 * route chosen while stopped survives to the next start.
	 */
	uint16_t in_src1;
	uint16_t in_src2;
	bool in_running;
	/* Owned by cs47l63_fault.c. fault_errors is a bitmask of
	 * audio_codec_error_type plus CS47L63_ERROR_CLOCK and
	 * CS47L63_ERROR_OUTPUT, OR'd into by cs47l63_fault_check() and zeroed
	 * only by cs47l63_fault_clear().
	 * fault_cb is the callback registered through the class API, or NULL.
	 */
	audio_codec_error_callback_t fault_cb;
	uint32_t fault_errors;
	/* Owned by cs47l63.c. The deferred check armed by start_output - it
	 * reads the FLL lock and completes the amplifier enable, both of which
	 * need a clock that does not exist yet when start_output runs - and the
	 * back-pointer its handler needs to reach the device it belongs to.
	 */
	const struct device *dev;
	struct k_work_delayable start_check;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_PRIV_H_ */
