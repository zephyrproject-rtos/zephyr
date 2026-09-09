/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio gain element header file.
 * @ingroup mpipe_aud_gain_elements
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_GAIN_H_
#define ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_GAIN_H_

/**
 * @defgroup mpipe_aud_gain_elements Gain
 * @ingroup mpipe_aud
 * @brief Audio gain transform elements.
 * @{
 */

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_transform.h>

/**
 * @brief Audio transform property identifiers
 *
 * Enumeration defining property IDs specific to transform elements.
 * These properties extend the base transform properties.
 */
enum mpipe_prop_aud_transform {
	/** Gain control property for audio level adjustment */
	MPIPE_PROP_AUD_TRANSFORM_GAIN = MPIPE_PROP_TRANSFORM_LAST,
};

/**
 * @struct mpipe_aud_gain
 * @brief Audio gain element structure
 *
 * This structure represents a gain transform element.
 * It provides audio volume control with percentage-based gain settings,
 * fixed-point arithmetic for efficient processing, and mute
 * functionality.
 */
struct mpipe_aud_gain {
	/** Base transform element structure */
	struct mpipe_transform transform;
	/** Gain level as percentage (0-1000, where 100 = unity gain) */
	int gain_percent;
	/** Gain value in Q16.16 fixed-point format for efficient computation */
	int32_t gain_fixed;
	/** Mute flag - when true, optimizes processing by bypassing gain calculations */
	bool mute;
	/** Bit width of audio samples (e.g., 16, 24, 32 bits) */
	uint32_t bit_width;
	/**
	 * Buffer pool config relayed from downstream to upstream. This element
	 * is in-place, so the same buffers flow through and the upstream pool
	 * must satisfy the downstream buffering requirement.
	 */
	struct mpipe_buffer_pool_config alloc_cfg;
};

/**
 * @brief Initialize an audio gain element.
 *
 * This function initializes the audio gain processing element with default
 * values and sets up the function pointers.
 *
 * @param aud_gain Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_aud_gain_init(struct mpipe_aud_gain *aud_gain, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_GAIN_H_ */
