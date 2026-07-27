/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio DMIC source element header file.
 */

#ifndef __MP_AUD_DMIC_SRC_H__
#define __MP_AUD_DMIC_SRC_H__

/**
 * @defgroup mp_aud_dmic_sources DMIC Sources
 * @ingroup mp_aud
 * @brief Digital microphone source elements.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/aud/mp_aud_buffer_pool.h>
#include <zephyr/mp/aud/mp_aud_src.h>

/**
 * @struct mp_aud_dmic_src
 * @brief Audio DMIC source element structure
 *
 * This structure represents a digital microphone source element.
 */
struct mp_aud_dmic_src {
	/** Base audio source structure */
	struct mp_aud_src aud_src;
	/** Buffer pool for managing audio data buffers */
	struct mp_aud_buffer_pool pool;
};

/**
 * @brief Initialize an audio DMIC source element
 *
 * This function initializes the digital microphone source element
 * with default values, sets up the function pointers and
 * configures the buffer pool.
 *
 * @param self Pointer to the mp_element structure to be initialized as a
 *             DMIC source element.
 */
void mp_aud_dmic_src_init(struct mp_element *self);

/** @} */

#endif /* __MP_AUD_DMIC_SRC_H__ */
