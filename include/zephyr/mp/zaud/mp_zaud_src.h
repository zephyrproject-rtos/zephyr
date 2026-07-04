/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio source element header file.
 */

#ifndef __MP_ZAUD_SRC_H__
#define __MP_ZAUD_SRC_H__

/**
 * @defgroup mp_zaud_sources Sources
 * @ingroup mp_zaud
 * @brief Audio source elements backed by audio devices.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/core/mp_src.h>

#include <zephyr/mp/zaud/mp_zaud.h>

/**
 * @struct mp_zaud_src
 * @brief Audio source element structure
 *
 * This structure represents an audio source element.
 */
struct mp_zaud_src {
	struct mp_src src;
	int (*get_audio_caps)(const struct device *dev, struct audio_caps *caps);
};

struct mp_caps *mp_zaud_src_supported_caps(struct mp_src *src);
void mp_zaud_src_update_caps(struct mp_src *src);

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
void mp_zaud_src_init(struct mp_element *self);

/** @} */

#endif /* __MP_ZAUD_SRC_H__ */
