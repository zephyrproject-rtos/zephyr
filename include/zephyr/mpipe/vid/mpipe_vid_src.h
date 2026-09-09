/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Video source element backed by a Zephyr video device.
 * @ingroup mpipe_vid_sources
 *
 * Captures video frames from a hardware video device and pushes them
 * into the pipeline as a source element.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_SRC_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_SRC_H_

/**
 * @defgroup mpipe_vid_sources Sources
 * @ingroup mpipe_vid
 * @brief Video source elements backed by Zephyr video devices.
 * @{
 */

#include <zephyr/mpipe/mpipe_src.h>

#include <zephyr/mpipe/vid/mpipe_vid_object.h>

/**
 * @brief Video source element structure.
 *
 * Extends @ref mpipe_src with a @ref mpipe_vid_object to capture video data
 * from a Zephyr video device.
 */
struct mpipe_vid_src {
	/** Base source element */
	struct mpipe_src src;
	/** Zephyr video object */
	struct mpipe_vid_object vid_obj;
};

/**
 * @brief Initialize a video source element.
 *
 * @param vid_src Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_vid_src_init(struct mpipe_vid_src *vid_src, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_SRC_H_ */
