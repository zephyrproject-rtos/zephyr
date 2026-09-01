/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2S source element header file.
 * @ingroup mpipe_aud_i2s_sources
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_I2S_SRC_H_
#define ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_I2S_SRC_H_

/**
 * @defgroup mpipe_aud_i2s_sources I2S Sources
 * @ingroup mpipe_aud
 * @brief I2S capture source elements.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mpipe/aud/mpipe_aud_buffer_pool.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_codec_sink.h>
#include <zephyr/mpipe/aud/mpipe_aud_src.h>

/**
 * @brief Audio I2S source property identifiers
 *
 * These properties extend the base audio source properties.
 */
enum mpipe_prop_aud_i2s_src {
	/** Optional capture codec device */
	MPIPE_PROP_AUD_I2S_SRC_CODEC_DEVICE = MPIPE_PROP_AUD_SRC_DEVICE + 1,
	/** Clock role configuration for the I2S receiver and optional codec */
	MPIPE_PROP_AUD_I2S_SRC_CLK_ROLE,
};

/**
 * @struct mpipe_aud_i2s_src
 * @brief Audio I2S source element structure
 *
 * Captures PCM frames from an I2S receiver. A capture codec is optional: with
 * one, its capture path is configured and its capabilities narrow the
 * negotiated format; without one the element drives a microphone that outputs
 * I2S directly.
 */
struct mpipe_aud_i2s_src {
	/** Base audio source structure (must be first) */
	struct mpipe_aud_src aud_src;
	/** Buffer pool for managing audio data buffers */
	struct mpipe_aud_buffer_pool pool;
	/** Optional capture codec device, NULL for a microphone with no codec */
	const struct device *codec_dev;
	/** Clock role configuration for the I2S receiver and optional codec */
	enum mpipe_aud_i2s_codec_clk_role clk_role;
};

/**
 * @brief Initialize an audio I2S source element.
 *
 * The I2S receiver is taken from the @c i2s-codec-rx devicetree alias and the
 * optional capture codec from the @c audio-codec-capture alias, which may be
 * absent. A device missing from the devicetree leaves the pointer NULL; set it
 * afterwards through the corresponding property.
 *
 * @param aud_i2s_src Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_aud_i2s_src_init(struct mpipe_aud_i2s_src *aud_i2s_src, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_I2S_SRC_H_ */
