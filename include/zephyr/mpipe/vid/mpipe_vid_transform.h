/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hardware-backed video transform element.
 * @ingroup mpipe_vid_transforms
 *
 * Wraps a Zephyr memory-to-memory video device to perform
 * hardware-accelerated operations such as format conversion and scaling.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_H_

/**
 * @defgroup mpipe_vid_transforms Transforms
 * @ingroup mpipe_vid
 * @brief Hardware-backed video transform elements.
 * @{
 */

#include <zephyr/mpipe/mpipe_transform.h>
#include <zephyr/mpipe/vid/mpipe_vid_object.h>

/**
 * @brief Hardware video transform element structure.
 *
 * Extends @ref mpipe_transform with separate input and output
 * @ref mpipe_vid_object instances, enabling memory-to-memory video
 * transformations through the Zephyr video subsystem.
 */
struct mpipe_vid_transform {
	/** Base transform element */
	struct mpipe_transform transform;
	/** Input video object for receiving video data */
	struct mpipe_vid_object vid_obj_in;
	/** Output video object for producing transformed video data */
	struct mpipe_vid_object vid_obj_out;
};

/**
 * @brief Initialize a hardware video transform element.
 *
 * @param vid_transform Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_vid_transform_init(struct mpipe_vid_transform *vid_transform, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_H_ */
