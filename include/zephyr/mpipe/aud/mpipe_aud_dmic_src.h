/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio DMIC source element header file.
 * @ingroup mpipe_aud_dmic_sources
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_DMIC_SRC_H_
#define ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_DMIC_SRC_H_

/**
 * @defgroup mpipe_aud_dmic_sources DMIC Sources
 * @ingroup mpipe_aud
 * @brief Digital microphone source elements.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mpipe/aud/mpipe_aud_buffer_pool.h>
#include <zephyr/mpipe/aud/mpipe_aud_src.h>

/**
 * @struct mpipe_aud_dmic_src
 * @brief Audio DMIC source element structure
 *
 * This structure represents a digital microphone source element.
 */
struct mpipe_aud_dmic_src {
	/** Base audio source structure */
	struct mpipe_aud_src aud_src;
	/** Buffer pool for managing audio data buffers */
	struct mpipe_aud_buffer_pool pool;
};

/**
 * @brief Initialize an audio DMIC source element
 *
 * This function initializes the digital microphone source element
 * with default values, sets up the function pointers and
 * configures the buffer pool.
 *
 * @param aud_dmic_src Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_aud_dmic_src_init(struct mpipe_aud_dmic_src *aud_dmic_src, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_AUD_MPIPE_AUD_DMIC_SRC_H_ */
