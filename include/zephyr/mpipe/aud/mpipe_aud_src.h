/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio source element header file.
 * @ingroup mpipe_aud_sources
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_SRC_H_
#define ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_SRC_H_

/**
 * @defgroup mpipe_aud_sources Sources
 * @ingroup mpipe_aud
 * @brief Audio source elements backed by audio devices.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mpipe/mpipe_src.h>

#include <zephyr/mpipe/aud/mpipe_aud.h>

/**
 * @brief Audio source property identifiers
 *
 * Enumeration defining property IDs specific to source elements.
 * These properties extend the base source properties.
 */
enum mpipe_prop_aud_src {
	/** Pointer to source memory slab for audio buffer management */
	MPIPE_PROP_AUD_SRC_SLAB_PTR = MPIPE_PROP_SRC_LAST,
	/** Audio source device */
	MPIPE_PROP_AUD_SRC_DEVICE,
};

/**
 * @struct mpipe_aud_src
 * @brief Audio source element structure
 *
 * This structure represents an audio source element.
 */
struct mpipe_aud_src {
	/** Base source element (must be first) */
	struct mpipe_src src;
	/**
	 * Read what the backing device supports. A derived source sets this to
	 * the accessor of its own device, and the base enumerates capabilities
	 * through it rather than holding a fixed set of its own.
	 */
	int (*get_audio_caps)(const struct device *dev, struct audio_caps *caps);
};

/**
 * @brief Serve the source pad's capabilities from the audio device.
 *
 * Installs the enumeration hook that answers a capability query by asking the
 * device through @c get_audio_caps, rather than from anything stored on the
 * element. A derived source calls this once its device is known, and again if
 * that device changes.
 *
 * @param src Pointer to the base source element of an @ref mpipe_aud_src.
 */
void mpipe_aud_src_update_caps(struct mpipe_src *src);

/**
 * @brief Initialize an audio source element
 *
 *
 * This function initializes the audio source element with default
 * values and sets up the function pointers.
 *
 * @param aud_src Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_aud_src_init(struct mpipe_aud_src *aud_src, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_SRC_H_ */
