/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zdisp sink element.
 */

#ifndef __MP_ZDISP_SINK_H__
#define __MP_ZDISP_SINK_H__

#include <zephyr/device.h>

#include <src/core/mp_sink.h>

#define MP_ZDISP_SINK(self) ((MpZdispSink *)self)

/**
 * @brief Display Sink structure
 *
 * This structure represents a display sink element that can render
 * image on a display device. It extends the base @ref MpSink
 * functionality with display-specific capabilities.
 */
typedef struct {
	/** Base sink element */
	MpSink sink;
	/** Display device instance */
	const struct device *display_dev;
} MpZdispSink;

#endif /* __MP_ZDISP_SINK_H__ */
