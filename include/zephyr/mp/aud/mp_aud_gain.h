/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio gain element header file.
 */

#ifndef __MP_AUD_GAIN_H__
#define __MP_AUD_GAIN_H__

/**
 * @defgroup mp_aud_gain_elements Gain
 * @ingroup mp_aud
 * @brief Audio gain transform elements.
 * @{
 */

#include <zephyr/mp/mp_transform.h>

/**
 * @brief Audio transform property identifiers
 *
 * Enumeration defining property IDs specific to transform elements.
 * These properties extend the base transform properties.
 */
enum mp_prop_aud_transform {
	/** Gain control property for audio level adjustment */
	MP_PROP_AUD_TRANSFORM_GAIN = MP_PROP_TRANSFORM_LAST,
};

/**
 * @struct mp_aud_gain
 * @brief Audio gain element structure
 *
 * This structure represents a gain transform element.
 * It provides audio volume control with percentage-based gain settings,
 * fixed-point arithmetic for efficient processing, and mute
 * functionality.
 */
struct mp_aud_gain {
	/** Base transform element structure */
	struct mp_transform transform;
	/** Gain level as percentage (0-1000, where 100 = unity gain) */
	int gain_percent;
	/** Gain value in Q16.16 fixed-point format for efficient computation */
	int32_t gain_fixed;
	/** Mute flag - when true, optimizes processing by bypassing gain calculations */
	bool mute;
	/** Bit width of audio samples (e.g., 16, 24, 32 bits) */
	uint32_t bit_width;
};

/**
 * @brief Initialize an audio gain element.
 *
 * This function initializes the audio gain processing element with default
 * values and sets up the function pointers.
 *
 * @param self Pointer to the mp_element structure to be initialized as a
 *             gain element.
 */
void mp_aud_gain_init(struct mp_element *self);

/** @} */

#endif /* __MP_AUD_GAIN_H__ */
