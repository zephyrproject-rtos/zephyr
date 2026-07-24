/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Video source element backed by a Zephyr video device.
 *
 * Captures video frames from a hardware video device and pushes them
 * into the pipeline as a source element.
 */

#ifndef ZEPHYR_INCLUDE_MP_VID_MP_VID_SRC_H_
#define ZEPHYR_INCLUDE_MP_VID_MP_VID_SRC_H_

/**
 * @defgroup mp_vid_sources Sources
 * @ingroup mp_vid
 * @brief Video source elements backed by Zephyr video devices.
 * @{
 */

#include <zephyr/mp/mp_src.h>

#include <zephyr/mp/vid/mp_vid_object.h>

/**
 * @brief Video source element structure.
 *
 * Extends @ref mp_src with a @ref mp_vid_object to capture video data
 * from a Zephyr video device.
 */
struct mp_vid_src {
	/** Base source element */
	struct mp_src src;
	/** Zephyr video object */
	struct mp_vid_object vid_obj;
};

/**
 * @brief Initialize a video source element.
 *
 * @param self Pointer to the @ref mp_element to initialize.
 */
void mp_vid_src_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_VID_MP_VID_SRC_H_ */
