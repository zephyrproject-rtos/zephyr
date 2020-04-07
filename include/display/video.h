/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/display.h>

#ifndef ZEPHYR_INCLUDE_DISPLAY_VIDEO_H_
#define ZEPHYR_INCLUDE_DISPLAY_VIDEO_H_

int display_video_write(const struct device *dev, const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			const void *buf);
int display_video_read(const struct device *dev, const uint16_t x,
		       const uint16_t y,
		       const struct display_buffer_descriptor *desc, void *buf);
void *display_video_get_framebuffer(const struct device *dev);
int display_video_blanking_off(const struct device *dev);
int display_video_blanking_on(const struct device *dev);
int display_video_set_brightness(const struct device *dev,
				 const uint8_t brightness);
int display_video_set_contrast(const struct device *dev,
			       const uint8_t contrast);
int display_video_set_pixel_format(const struct device *dev,
				const enum display_pixel_format pixel_format);
int display_video_set_orientation(const struct device *dev,
				  const enum display_orientation orientation);
void display_video_get_capabilities(const struct device *dev,
				    struct display_capabilities *capabilities);
void display_video_init(struct device *dev);

#endif /* ZEPHYR_INCLUDE_DISPLAY_VIDEO_H_ */
