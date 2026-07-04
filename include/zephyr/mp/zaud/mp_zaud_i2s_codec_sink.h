/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2S codec sink element header file.
 */

#ifndef __MP_ZAUD_I2S_CODEC_SINK_H__
#define __MP_ZAUD_I2S_CODEC_SINK_H__

/**
 * @defgroup mp_zaud_i2s_codec_sinks I2S Codec Sinks
 * @ingroup mp_zaud
 * @brief Audio sink elements backed by I2S and codec devices.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/core/mp_sink.h>

/**
 * @enum mp_zaud_i2s_codec_clk_role
 * @brief Clock role configuration for I2S and codec devices.
 *
 * Defines whether the I2S (SAI) or codec device acts as the clock
 * controller or target for frame and bit clocks.
 */
enum mp_zaud_i2s_codec_clk_role {
	/** I2S (SAI) is controller, codec is target */
	MP_ZAUD_I2S_CONTROLLER_CODEC_TARGET,
	/** I2S (SAI) is target. codec is controller */
	MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER,
};

/**
 * @struct mp_zaud_i2s_codec_sink
 * @brief Audio I2S codec sink element structure.
 *
 * This structure represents an audio sink element that outputs audio data
 * through an I2S interface to an external codec device. It manages the
 * I2S communication and codec control.
 */
struct mp_zaud_i2s_codec_sink {
	/** Base sink element structure */
	struct mp_sink sink;
	/** I2S device instance for audio data transmission */
	const struct device *i2s_dev;
	/** Codec device instance for configuration */
	const struct device *codec_dev;
	/** Memory slab for audio buffer allocation */
	struct k_mem_slab *mem_slab;
	/** Number of buffers written at the beginning of the stream */
	uint8_t count;
	/** Number of queued buffers required before starting the stream */
	uint8_t start_threshold;
	/** Flag indicating if the sink has been started */
	bool started;
	/** Clock role configuration for I2S and codec */
	enum mp_zaud_i2s_codec_clk_role clk_role;
};

/**
 * @brief Initialize an audio I2S codec sink element.
 *
 * This function initializes the I2S codec sink element with default
 * values and sets up the function pointers.
 *
 * The function expects the following device tree nodes:
 * - i2s_codec_tx alias for I2S transmission device
 * - audio_codec node label for codec configuration device
 *
 * @param self Pointer to the mp_element structure to be initialized as an
 *             I2S codec sink element.
 *
 * @note This function will log errors and return early if required devices
 *       are not ready or not found in device tree.
 */
void mp_zaud_i2s_codec_sink_init(struct mp_element *self);

/** @} */

#endif /* __MP_ZAUD_I2S_CODEC_SINK_H__ */
