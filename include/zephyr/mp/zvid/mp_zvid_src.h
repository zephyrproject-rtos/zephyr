/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Video source element backed by a Zephyr video device.
 *
 * Captures video frames from a hardware video device and pushes them
 * into the pipeline as a source element.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_SRC_H_
#define ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_SRC_H_

#include <zephyr/mp/core/mp_src.h>

#include <zephyr/mp/zvid/mp_zvid_object.h>

/**
 * @brief Video source element structure.
 *
 * Extends @ref mp_src with a @ref mp_zvid_object to capture video data
 * from a Zephyr video device.
 */
struct mp_zvid_src {
	/** Base source element */
	struct mp_src src;
	/** Zephyr video object */
	struct mp_zvid_object zvid_obj;
};

/**
 * @brief Initialize a video source element.
 *
 * @param self Pointer to the @ref mp_element to initialize.
 */
void mp_zvid_src_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_SRC_H_ */
