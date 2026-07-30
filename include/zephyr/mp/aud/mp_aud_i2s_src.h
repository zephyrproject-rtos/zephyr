/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2S source element header file.
 */

#ifndef __MP_AUD_I2S_SRC_H__
#define __MP_AUD_I2S_SRC_H__

/**
 * @defgroup mp_aud_i2s_sources I2S Sources
 * @ingroup mp_aud
 * @brief I2S capture source elements.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/aud/mp_aud_buffer_pool.h>
#include <zephyr/mp/aud/mp_aud_i2s_codec_sink.h>
#include <zephyr/mp/aud/mp_aud_src.h>

/**
 * @brief Audio I2S source property identifiers
 *
 * Enumeration defining property IDs specific to the I2S source element.
 * These properties extend the base audio source properties.
 */
enum mp_prop_aud_i2s_src {
	/** Optional capture codec device */
	MP_PROP_AUD_I2S_SRC_CODEC_DEVICE = MP_PROP_AUD_SRC_LAST,
	/** Clock role configuration for the I2S receiver and optional codec */
	MP_PROP_AUD_I2S_SRC_CLK_ROLE,
};

/**
 * @struct mp_aud_i2s_src
 * @brief Audio I2S source element structure
 *
 * This structure represents an I2S capture source element. It captures PCM
 * frames from an I2S receiver.
 *
 * A capture codec is optional: with one, its capture path is configured and
 * its capabilities are taken into account when negotiating a format. Without
 * one the element drives a microphone that outputs I2S directly.
 */
struct mp_aud_i2s_src {
	/** Base audio source structure */
	struct mp_aud_src aud_src;
	/** Buffer pool for managing audio data buffers */
	struct mp_aud_buffer_pool pool;
	/** Optional capture codec device, NULL for a microphone with no codec */
	const struct device *codec_dev;
	/** Clock role configuration for the I2S receiver and optional codec */
	enum mp_aud_i2s_codec_clk_role clk_role;
};

/**
 * @brief Initialize an audio I2S source element
 *
 * This function initializes the I2S source element with default values, sets
 * up the function pointers and configures the buffer pool.
 *
 * The I2S receiver is taken from the @c i2s-codec-rx devicetree alias and the
 * optional capture codec from the @c audio-codec-capture alias, which may be
 * absent. Both are aliases so a board can point them at its own nodes without
 * the element having to know the node names.
 *
 * @param self Pointer to the mp_element structure to be initialized as an
 *             I2S source element.
 */
void mp_aud_i2s_src_init(struct mp_element *self);

/** @} */

#endif /* __MP_AUD_I2S_SRC_H__ */
