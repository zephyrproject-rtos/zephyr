/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Caps filter element.
 * @ingroup mpipe_caps_filter
 *
 * This element does not modify data; it constrains the format negotiated on
 * its link.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_CAPS_FILTER_H_
#define ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_CAPS_FILTER_H_

/**
 * @defgroup mpipe_caps_filter Caps Filters
 * @ingroup mpipe_base
 * @brief Transform elements that constrain negotiated capabilities.
 * @{
 */

#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_transform.h>

/**
 * @brief Caps filter property identifiers
 */
enum {
	/** Caps ID property */
	MPIPE_PROP_BASE_CAPS_FILTER_CAPS = MPIPE_PROP_TRANSFORM_LAST,
};

/**
 * @brief Caps filter element
 *
 * A transform that pins the capability negotiated on its link to the one the
 * application configured, without touching the data.
 */
struct mpipe_caps_filter {
	/** Base transform element */
	struct mpipe_transform transform;
	/** Upstream source pad that the sink pad was linked to */
	struct mpipe_pad *saved_sink_peer;
	/** Downstream sink pad that the source pad was linked to */
	struct mpipe_pad *saved_src_peer;
	/** Configured filter, re-applied to the pads on PAUSED -> READY */
	struct mpipe_structure filter_caps;
};

/**
 * @brief Initialize a caps filter element
 *
 * @param caps_filter Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_caps_filter_init(struct mpipe_caps_filter *caps_filter, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_CAPS_FILTER_H_ */
