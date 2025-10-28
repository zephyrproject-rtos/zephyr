/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio DMIC source element header file.
 */

#ifndef __MP_ZAUD_DMIC_SRC_H__
#define __MP_ZAUD_DMIC_SRC_H__

#include <zephyr/device.h>

#include "mp_zaud_src.h"

/** @brief Cast object pointer to mp_zaud_dmic_src pointer */
#define MP_ZAUD_DMIC_SRC(self) ((struct mp_zaud_dmic_src *)self)

/**
 * @struct mp_zaud_dmic_src
 * @brief Audio DMIC source element structure
 *
 * This structure represents a digital microphone source element.
 */
struct mp_zaud_dmic_src {
	/** Base audio source structure */
	struct mp_zaud_src zaud_src;
	/** Buffer pool for managing audio data buffers */
	struct mp_zaud_buffer_pool pool;
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
void mp_zaud_dmic_src_init(struct mp_element *self);

#endif /* __MP_ZAUD_DMIC_SRC_H__ */
