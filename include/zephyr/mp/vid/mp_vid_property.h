/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp_vid_properties
 * @brief Property identifiers for the vid plugin.
 *
 * Extends the base source and transform property enumerations with
 * video-specific property keys.
 */

#ifndef ZEPHYR_INCLUDE_MP_VID_MP_VID_PROPERTY_H_
#define ZEPHYR_INCLUDE_MP_VID_MP_VID_PROPERTY_H_

/**
 * @defgroup mp_vid_properties Properties
 * @ingroup mp_vid
 * @brief Property identifiers for vid elements.
 * @{
 */

#include <zephyr/sys/util.h>

#include <zephyr/mp/mp_src.h>
#include <zephyr/mp/mp_transform.h>

/**
 * @brief Video property identifiers.
 *
 * Starts from MAX(@ref MP_PROP_SRC_LAST, @ref MP_PROP_TRANSFORM_LAST) + 1 to
 * avoid conflicts with base source and transform properties.
 */
enum mp_prop_vid {
	/** Video device property */
	MP_PROP_VID_DEVICE = MAX((int)MP_PROP_SRC_LAST, (int)MP_PROP_TRANSFORM_LAST),
	/** Crop selection target property */
	MP_PROP_VID_CROP,
};

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_VID_MP_VID_PROPERTY_H_ */
