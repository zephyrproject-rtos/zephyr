/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zvide source element.
 */

#ifndef __MP_ZVID_SRC_H__
#define __MP_ZVID_SRC_H__

#include <src/core/mp_element.h>
#include <src/core/mp_src.h>

#include "mp_zvid_object.h"

#define MP_ZVID_SRC(self) ((MpZvidSrc *)self)

/**
 * @brief Video Source Element Structure
 *
 * This structure represents a video source element that captures video data
 * from a video device and provides. It extends the base @ref MpSrc functionality
 * with specific video handling.
 */
typedef struct {
	/** Base source element */
	MpSrc src;
	/** Zephyr video object */
	MpZvidObject zvid_obj;
} MpZvidSrc;

/**
 * @brief Initialize a video source element
 *
 * @param self Pointer to the @ref MpElement to initialize
 */
void mp_zvid_src_init(MpElement *self);

#endif /* __MP_ZVID_SRC_H__ */
