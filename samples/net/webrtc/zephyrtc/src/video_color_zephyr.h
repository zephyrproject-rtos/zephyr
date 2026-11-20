/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_VIDEO_COLOR_ZEPHYR_H_
#define ZEPHYRTC_VIDEO_COLOR_ZEPHYR_H_

#include <stdint.h>

#define VIDEO_COLOR_MAX_SCALE 3U

void video_color_bgrx32_to_i420(const uint8_t *bgrx, uint8_t *i420);
int video_color_rgb565_dimensions(uint32_t rotation, uint32_t scale, uint32_t *width,
				  uint32_t *height);
int video_color_i420_to_rgb565(const uint8_t *i420, uint16_t *rgb565, uint32_t rotation,
			       uint32_t scale);

#endif /* ZEPHYRTC_VIDEO_COLOR_ZEPHYR_H_ */
