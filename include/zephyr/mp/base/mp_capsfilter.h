/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Capsfilter element.
 *
 * This element does not modify data, but used to enforce limitations on the data format.
 *
 */

#ifndef ZEPHYR_INCLUDE_MP_BASE_MP_BASE_CAPSFILTER_H_
#define ZEPHYR_INCLUDE_MP_BASE_MP_BASE_CAPSFILTER_H_

/**
 * @defgroup mp_base base
 * @ingroup mp_plugins
 * @brief Base plugin elements shared across MultimediaPipe graphs.
 */

/**
 * @defgroup mp_capsfilter Caps Filters
 * @ingroup mp_base
 * @brief Transform elements that constrain negotiated capabilities.
 * @{
 */

#include <zephyr/mp/mp_element.h>
#include <zephyr/mp/mp_transform.h>

/**
 * @brief Caps filter Property Identifiers
 *
 * Defined property identifiers specific to the capsfilter element. These
 * properties extend the base transform properties defined in @ref mp_prop_transform.
 *
 * The enumeration starts from MP_PROP_TRANSFORM_LAST to ensure no
 * conflicts with base transform properties.
 */
enum {
	/** Caps ID property */
	MP_PROP_BASE_CAPSFILTER_CAPS = MP_PROP_TRANSFORM_LAST,
};

/**
 * @brief Capsfilter Element Structure
 *
 */
struct mp_caps_filter {
	/** Base transform element */
	struct mp_transform transform;
	/** Upstream source pad that the sink pad was linked to */
	struct mp_pad *saved_sink_peer;
	/** Downstream sink pad that the source pad was linked to */
	struct mp_pad *saved_src_peer;
};

/**
 * @brief Initialize a caps filter element
 *
 * @param self Pointer to the @ref mp_element to initialize as a caps filter
 */
void mp_caps_filter_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_BASE_MP_BASE_CAPSFILTER_H_ */
