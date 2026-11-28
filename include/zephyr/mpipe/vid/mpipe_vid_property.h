/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mpipe_vid_properties
 * @brief Property identifiers for the vid plugin.
 *
 * Extends the base source and transform property enumerations with
 * video-specific property keys.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_PROPERTY_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_PROPERTY_H_

/**
 * @defgroup mpipe_vid_properties Properties
 * @ingroup mpipe_vid
 * @brief Property identifiers for vid elements.
 * @{
 */

#include <zephyr/sys/util.h>

#include <zephyr/mpipe/mpipe_src.h>
#include <zephyr/mpipe/mpipe_transform.h>

/**
 * @brief Video property identifiers.
 *
 * Starts from MAX(@ref MPIPE_PROP_SRC_LAST, @ref MPIPE_PROP_TRANSFORM_LAST) to
 * avoid conflicts with base source and transform properties.
 */
enum mpipe_prop_vid {
	/** Video device property */
	MPIPE_PROP_VID_DEVICE = MAX((int)MPIPE_PROP_SRC_LAST, (int)MPIPE_PROP_TRANSFORM_LAST),
	/** Crop selection target property */
	MPIPE_PROP_VID_CROP,
};

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_PROPERTY_H_ */
