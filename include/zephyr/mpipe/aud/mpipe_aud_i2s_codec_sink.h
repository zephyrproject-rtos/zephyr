/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2S codec sink element header file.
 * @ingroup mpipe_aud_i2s_codec_sinks
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_I2S_CODEC_SINK_H_
#define ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_I2S_CODEC_SINK_H_

/**
 * @defgroup mpipe_aud_i2s_codec_sinks I2S Codec Sinks
 * @ingroup mpipe_aud
 * @brief Audio sink elements backed by I2S and codec devices.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mpipe/mpipe_sink.h>

/**
 * @brief Audio sink property identifiers
 *
 * Enumeration defining property IDs specific to sink elements.
 * These properties extend the base sink properties.
 */
enum mpipe_prop_aud_sink {
	/** Pointer to sink memory slab for audio buffer management */
	MPIPE_PROP_AUD_SINK_SLAB_PTR = MPIPE_PROP_SINK_LAST,
	/** Clock role configuration for audio sink (controller/target) */
	MPIPE_PROP_AUD_SINK_CLK_ROLE,
	/** I2S (SAI) sink device */
	MPIPE_PROP_AUD_SINK_I2S_DEVICE,
	/** Codec sink device */
	MPIPE_PROP_AUD_SINK_CODEC_DEVICE,
};

/**
 * @enum mpipe_aud_i2s_codec_clk_role
 * @brief Clock role configuration for I2S and codec devices.
 *
 * Defines whether the I2S (SAI) or codec device acts as the clock
 * controller or target for frame and bit clocks.
 */
enum mpipe_aud_i2s_codec_clk_role {
	/** I2S (SAI) is controller, codec is target */
	MPIPE_AUD_I2S_CONTROLLER_CODEC_TARGET,
	/** I2S (SAI) is target. codec is controller */
	MPIPE_AUD_I2S_TARGET_CODEC_CONTROLLER,
};

/**
 * @struct mpipe_aud_i2s_codec_sink
 * @brief Audio I2S codec sink element structure.
 *
 * This structure represents an audio sink element that outputs audio data
 * through an I2S interface to an external codec device. It manages the
 * I2S communication and codec control.
 */
struct mpipe_aud_i2s_codec_sink {
	/** Base sink element structure */
	struct mpipe_sink sink;
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
	enum mpipe_aud_i2s_codec_clk_role clk_role;
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
 * @param aud_i2s_codec_sink Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @note A device missing from the devicetree leaves the corresponding pointer
 *       NULL rather than failing; set it afterwards through
 *       MPIPE_PROP_AUD_SINK_I2S_DEVICE or MPIPE_PROP_AUD_SINK_CODEC_DEVICE.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_aud_i2s_codec_sink_init(struct mpipe_aud_i2s_codec_sink *aud_i2s_codec_sink, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_I2S_CODEC_SINK_H_ */
