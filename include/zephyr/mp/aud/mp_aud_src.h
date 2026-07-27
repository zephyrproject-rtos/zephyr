/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio source element header file.
 */

#ifndef __MP_AUD_SRC_H__
#define __MP_AUD_SRC_H__

/**
 * @defgroup mp_aud_sources Sources
 * @ingroup mp_aud
 * @brief Audio source elements backed by audio devices.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/mp_src.h>

#include <zephyr/mp/aud/mp_aud.h>

/**
 * @brief Audio source property identifiers
 *
 * Enumeration defining property IDs specific to source elements.
 * These properties extend the base source properties.
 */
enum mp_prop_aud_src {
	/** Pointer to source memory slab for audio buffer management */
	MP_PROP_AUD_SRC_SLAB_PTR = MP_PROP_SRC_LAST,
	/** Audio source device */
	MP_PROP_AUD_SRC_DEVICE,
};

/**
 * @struct mp_aud_src
 * @brief Audio source element structure
 *
 * This structure represents an audio source element.
 */
struct mp_aud_src {
	struct mp_src src;
	int (*get_audio_caps)(const struct device *dev, struct audio_caps *caps);
};

struct mp_caps *mp_aud_src_supported_caps(struct mp_src *src);
void mp_aud_src_update_caps(struct mp_src *src);

/**
 * @brief Initialize an audio source element
 *
 *
 * This function initializes the audio source element with default
 * values and sets up the function pointers.
 *
 * @param self Pointer to the mp_element structure to be initialized as an
 *             audio source element.
 */
void mp_aud_src_init(struct mp_element *self);

/** @} */

#endif /* __MP_AUD_SRC_H__ */
