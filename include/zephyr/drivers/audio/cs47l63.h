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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup cs47l63_interface CS47L63
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

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_AUDIO_CS47L63_H_ */
