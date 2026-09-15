/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mpipe_disp_sinks
 * @brief Display sink element for the mpipe disp plugin.
 *
 * Provides a sink element that renders video frames to a Zephyr display
 * device, supporting partial frame updates.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_DISP_MPIPE_DISP_SINK_H_
#define ZEPHYR_INCLUDE_MPIPE_DISP_MPIPE_DISP_SINK_H_

/**
 * @defgroup mpipe_disp Display
 * @ingroup mpipe_plugins
 * @brief The element that puts frames on a screen.
 *
 * The display plugin sits on Zephyr's display API and terminates a video graph.
 * It is a sink in the strict sense: frames arrive and nothing leaves.
 *
 * What it accepts comes from the panel rather than from the element, so its
 * capabilities are enumerated from the display's own pixel-format bitmask and
 * its resolution, which is why a graph ending here usually needs a converter in
 * front of it unless the source can already produce what the panel takes.
 */

/**
 * @defgroup mpipe_disp_sinks Sinks
 * @ingroup mpipe_disp
 * @brief Display sink elements.
 * @{
 */

#include <zephyr/device.h>

#include <zephyr/mpipe/mpipe_sink.h>

/**
 * @brief Display sink property identifiers.
 *
 * Extends the base sink properties defined in @ref mpipe_prop_sink.
 * Enumeration starts from @ref MPIPE_PROP_SINK_LAST to avoid conflicts.
 */
enum {
	/** Display device property (const struct device *). */
	MPIPE_PROP_DISP_SINK_DEVICE = MPIPE_PROP_SINK_LAST,
};

/**
 * @brief Display sink element.
 *
 * Extends the base @ref mpipe_sink with display-specific capabilities.
 * Renders incoming video buffers to a Zephyr display device.
 */
struct mpipe_disp_sink {
	/** Base sink element. */
	struct mpipe_sink sink;
	/** Display device instance. */
	const struct device *display_dev;
};

/**
 * @brief Initialize a display sink element.
 *
 * @param disp_sink Element to initialize.
 * @param id        Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_disp_sink_init(struct mpipe_disp_sink *disp_sink, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_DISP_MPIPE_DISP_SINK_H_ */
