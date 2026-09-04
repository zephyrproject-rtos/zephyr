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
 * This element does not modify data, but used to enforce limitations on the data format.
 *
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_CAPS_FILTER_H_
#define ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_CAPS_FILTER_H_

/**
 * @defgroup mpipe_base Base Elements
 * @ingroup mpipe_plugins
 * @brief Media-agnostic elements that shape a graph rather than its data.
 *
 * The base plugin holds the elements that belong to no media domain. They do
 * not look at what a buffer contains: they constrain what a link may carry,
 * split a graph into branches, or split it across threads. Every domain needs
 * them, so they live together rather than being duplicated per domain.
 */

/**
 * @defgroup mpipe_caps_filter Caps Filters
 * @ingroup mpipe_base
 * @brief Transform elements that constrain negotiated capabilities.
 * @{
 */

#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_transform.h>

/**
 * @brief Caps filter Property Identifiers
 *
 * Defined property identifiers specific to the caps_filter element. These
 * properties extend the base transform properties defined in @ref mpipe_prop_transform.
 *
 * The enumeration starts from MPIPE_PROP_TRANSFORM_LAST to ensure no
 * conflicts with base transform properties.
 */
enum {
	/** Caps ID property */
	MPIPE_PROP_BASE_CAPS_FILTER_CAPS = MPIPE_PROP_TRANSFORM_LAST,
};

/**
 * @brief Caps filter element.
 *
 * Holds the filter the application configured, and the two peers the element
 * was linked to. It keeps its own copy of the filter because the pads are reset
 * on teardown: without it, a second run would negotiate unconstrained.
 *
 * The saved peers are what let the element take itself out of the graph once
 * the format is settled and put itself back on teardown, so that it costs
 * nothing on the buffer path.
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
