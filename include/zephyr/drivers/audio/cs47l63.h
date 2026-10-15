/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API extensions for the CS47L63 audio codec
 * @ingroup cs47l63_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AUDIO_CS47L63_H_
#define ZEPHYR_INCLUDE_DRIVERS_AUDIO_CS47L63_H_

#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup cs47l63_interface CS47L63
 * @since 4.6
 * @version 0.1.0
 * @ingroup audio_codec_interface_ext
 * @brief CS47L63 audio codec
 * @{
 */

/** @brief Input terminal IDs for audio_codec_route_input(). */
enum cs47l63_input {
	CS47L63_INPUT_LINE = 0, /**< IN2L/IN2R, the analog line pair. */
	CS47L63_INPUT_PDM = 1,  /**< IN1L/IN1R in digital mode, a PDM microphone. */
};

/** @brief Output terminal IDs for audio_codec_route_output(). */
enum cs47l63_output {
	CS47L63_OUTPUT_HP = 0, /**< OUTP/OUTN, the differential headphone driver. */
};

/**
 * @name Error callback bits
 *
 * Bits this driver sets in the error callback's mask besides
 * @ref audio_codec_error_type, which uses BIT(0) to BIT(4).
 *
 * Faults are polled when the output starts and when its volume or mute is set. With
 * irq-gpios they are also reported when the part raises them, unless the controller has
 * no edge interrupts or the line stays asserted. Stop the output before
 * stopping its clock: a clock lost under a running output is reported as
 * CS47L63_ERROR_CLOCK.
 *
 * The callback runs with the codec locked, in the calling thread or on the
 * system workqueue. It may call this codec, but must not wait on another
 * thread that does.
 * @{
 */

/** Loss of SYSCLK or FLL1 lock. */
#define CS47L63_ERROR_CLOCK BIT(5)

/** The output stage did not start: a dead headphone driver. */
#define CS47L63_ERROR_OUTPUT BIT(6)

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_AUDIO_CS47L63_H_ */
