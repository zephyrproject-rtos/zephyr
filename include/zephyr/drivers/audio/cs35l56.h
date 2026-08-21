/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file providing the API for the CS35L56 smart amplifier
 * @ingroup cs35l56_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AUDIO_CS35L56_H_
#define ZEPHYR_INCLUDE_DRIVERS_AUDIO_CS35L56_H_

#include <zephyr/audio/codec.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup cs35l56_interface CS35L56
 * @ingroup audio_codec_interface_ext
 * @brief CS35L56 smart amplifier
 * @{
 */

/**
 * @brief Custom channels for asp1 transmission
 */
typedef enum {
	CS35L56_CHANNEL_ASP1_TX1 = AUDIO_CHANNEL_PRIV_START, /**< ASP1 TX1 channel index */
	CS35L56_CHANNEL_ASP1_TX2,                            /**< ASP1 TX2 channel index */
	CS35L56_CHANNEL_ASP1_TX3,                            /**< ASP1 TX3 channel index */
	CS35L56_CHANNEL_ASP1_TX4                             /**< ASP1 TX4 channel index */
} cs35l56_channel_t;

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_INCLUDE_DRIVERS_AUDIO_CS35L56_H_ */
