/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp_disp_sinks
 * @brief Display sink element for the MP disp plugin.
 *
 * Provides a sink element that renders video frames to a Zephyr display
 * device, supporting partial frame updates and configurable display regions.
 */

#ifndef ZEPHYR_INCLUDE_MP_DISP_MP_DISP_SINK_H_
#define ZEPHYR_INCLUDE_MP_DISP_MP_DISP_SINK_H_

/**
 * @defgroup mp_disp disp
 * @ingroup mp_plugins
 * @brief Display sink elements and related properties.
 */

/**
 * @defgroup mp_disp_sinks Sinks
 * @ingroup mp_disp
 * @brief Display sink elements.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mp/mp_sink.h>

/**
 * @brief Display sink property identifiers.
 *
 * Extends the base sink properties defined in @ref mp_prop_sink.
 * Enumeration starts from @ref MP_PROP_SINK_LAST to avoid conflicts.
 */
enum {
	/** Display device property (const struct device *). */
	MP_PROP_DISP_SINK_DEVICE = MP_PROP_SINK_LAST,
};

/**
 * @brief Display sink element.
 *
 * Extends the base @ref mp_sink with display-specific capabilities.
 * Renders incoming video buffers to a Zephyr display device.
 */
struct mp_disp_sink {
	/** Base sink element. */
	struct mp_sink sink;
	/** Display device instance. */
	const struct device *display_dev;
};

/**
 * @brief Initialize a display sink element.
 *
 * @param self Pointer to the element to initialize.
 */
void mp_disp_sink_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_DISP_MP_DISP_SINK_H_ */
