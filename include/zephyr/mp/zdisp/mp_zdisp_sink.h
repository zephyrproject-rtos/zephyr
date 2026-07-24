/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Display sink element for the MP zdisp plugin.
 *
 * Provides a sink element that renders video frames to a Zephyr display
 * device, supporting partial frame updates and configurable display regions.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZDISP_MP_ZDISP_SINK_H_
#define ZEPHYR_INCLUDE_MP_ZDISP_MP_ZDISP_SINK_H_

#include <zephyr/device.h>

#include <zephyr/mp/core/mp_sink.h>

/**
 * @brief Display sink element.
 *
 * Extends the base @ref mp_sink with display-specific capabilities.
 * Renders incoming video buffers to a Zephyr display device.
 */
struct mp_zdisp_sink {
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
void mp_zdisp_sink_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZDISP_MP_ZDISP_SINK_H_ */
