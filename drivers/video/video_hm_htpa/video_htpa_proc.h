/*
 * Copyright (c) 2026 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_VIDEO_HM_HTPA_VIDEO_HTPA_PROC_H_
#define ZEPHYR_DRIVERS_VIDEO_VIDEO_HM_HTPA_VIDEO_HTPA_PROC_H_

#include <stdint.h>

#define HTPA_HISTOGRAM_BIN_COUNT 256U

struct video_buffer;

/**
 * @brief Process a raw sensor frame into an output video buffer.
 *
 * @param width Frame width in pixels.
 * @param height Frame height in pixels.
 * @param histogram Histogram workspace, or NULL when histogram autoscaling is disabled.
 * @param frame Raw sensor frame.
 * @param vbuf Output video buffer.
 */
void htpa_process_frame(uint16_t width, uint16_t height, uint16_t *histogram,
			const struct video_buffer *frame, struct video_buffer *vbuf);

#endif /* ZEPHYR_DRIVERS_VIDEO_VIDEO_HM_HTPA_VIDEO_HTPA_PROC_H_ */
