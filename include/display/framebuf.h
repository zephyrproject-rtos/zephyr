/*
 * Copyright (c) 2019 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DISPLAY_FRAMEBUF_H_
#define ZEPHYR_INCLUDE_DISPLAY_FRAMEBUF_H_

#include <drivers/display.h>

extern const struct display_driver_api framebuf_display_api;

struct framebuf_dev_data {
	void			  *buffer;
	u32_t			  pitch;
	u16_t			  width;
	u16_t			  height;
};

#define FRAMEBUF_DATA(dev) ((struct framebuf_dev_data *) ((dev)->driver_data))

#endif /* ZEPHYR_INCLUDE_DISPLAY_FRAMEBUF_H_ */
