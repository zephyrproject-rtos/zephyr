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

#define MP_ZVID_SRC(self) ((struct mp_zvid_src *)self)

/**
 * @brief Video Source Element Structure
 *
 * This structure represents a video source element that captures video data
 * from a video device and provides. It extends the base @ref struct mp_src functionality
 * with specific video handling.
 */
struct mp_zvid_src {
	/** Base source element */
	struct mp_src src;
	/** Zephyr video object */
	struct mp_zvid_object zvid_obj;
};

/**
 * @brief Initialize a video source element
 *
 * @param self Pointer to the @ref struct mp_element to initialize
 */
void mp_zvid_src_init(struct mp_element *self);

#endif /* __MP_ZVID_SRC_H__ */
