/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2S codec sink element header file.
 */

#ifndef __MP_AUD_I2S_CODEC_SINK_H__
#define __MP_AUD_I2S_CODEC_SINK_H__

/**
 * @defgroup mp_aud_i2s_codec_sinks I2S Codec Sinks
 * @ingroup mp_aud
 * @brief Audio sink elements backed by I2S and codec devices.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/mp_sink.h>

/**
 * @brief Audio sink property identifiers
 *
 * Enumeration defining property IDs specific to sink elements.
 * These properties extend the base sink properties.
 */
enum mp_prop_aud_sink {
	/** Pointer to sink memory slab for audio buffer management */
	MP_PROP_AUD_SINK_SLAB_PTR = MP_PROP_SINK_LAST,
	/** Clock role configuration for audio sink (controller/target) */
	MP_PROP_AUD_SINK_CLK_ROLE,
	/** I2S (SAI) sink device */
	MP_PROP_AUD_SINK_I2S_DEVICE,
	/** Codec sink device */
	MP_PROP_AUD_SINK_CODEC_DEVICE,
};

/**
 * @enum mp_aud_i2s_codec_clk_role
 * @brief Clock role configuration for I2S and codec devices.
 *
 * Defines whether the I2S (SAI) or codec device acts as the clock
 * controller or target for frame and bit clocks.
 */
enum mp_aud_i2s_codec_clk_role {
	/** I2S (SAI) is controller, codec is target */
	MP_AUD_I2S_CONTROLLER_CODEC_TARGET,
	/** I2S (SAI) is target. codec is controller */
	MP_AUD_I2S_TARGET_CODEC_CONTROLLER,
};

/**
 * @struct mp_aud_i2s_codec_sink
 * @brief Audio I2S codec sink element structure.
 *
 * This structure represents an audio sink element that outputs audio data
 * through an I2S interface to an external codec device. It manages the
 * I2S communication and codec control.
 */
struct mp_aud_i2s_codec_sink {
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
	enum mp_aud_i2s_codec_clk_role clk_role;
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
void mp_aud_i2s_codec_sink_init(struct mp_element *self);

/** @} */

#endif /* __MP_AUD_I2S_CODEC_SINK_H__ */
