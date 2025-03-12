/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_IMAGER_H_
#define ZEPHYR_DRIVERS_VIDEO_IMAGER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>

/*
 * Table entry for an "imaging mode" that can be applied to an imager to configure a particular
 * framerate, resolution, and pixel format. The index of the table of modes is meant to match
 * the index of the table of formats.
 */
struct video_imager_mode {
	/* FPS for this mode */
	uint16_t fps;
	/* Multiple lists of registers to allow sharing common sets of registers across modes. */
	const struct video_cci_reg *regs[3];
};

/*
 * A video imager device is expected to have dev->data point to this structure.
 * In order to support custom data structure, it is possible to store an extra pointer
 * in the dev->config struct. See existing drivers for an example.
 */
struct video_imager_data {
	/* Index of the currently active format in both modes[] and fmts[] */
	int fmt_id;
	/* Currently active video format */
	struct video_format fmt;
	/* List of all formats supported by this sensor */
	const struct video_format_cap *fmts;
	/* Currently active operating mode as defined above */
	const struct video_imager_mode *mode;
	/* Array of modes tables, one table per format cap lislted by "fmts" */
	const struct video_imager_mode **modes;
	/* I2C device to write the registers to */
	struct i2c_dt_spec i2c;
};

/*
 * Initialize one row of a @struct video_format_cap with fixed width and height
 * The minimum and maximum are the same for both width and height fields.
 */
#define VIDEO_IMAGER_FORMAT_CAP(pixfmt, width, height)                                             \
	{                                                                                          \
		.width_min = (width), .width_max = (width), .width_step = 0,                       \
		.height_min = (height), .height_max = (height), .height_step = 0,                  \
		.pixelformat = (pixfmt),                                                           \
	}

/*
 * Function which will set the operating mode (as defined above) of the imager.
 */
int video_imager_set_mode(const struct device *dev, const struct video_imager_mode *mode);

/*
 * Default implementations that can be passed directly to the struct video_api, or called by the
 * driver implementation.
 */
int video_imager_set_frmival(const struct device *dev, enum video_endpoint_id ep,
			     struct video_frmival *frmival);
int video_imager_get_frmival(const struct device *dev, enum video_endpoint_id ep,
			     struct video_frmival *frmival);
int video_imager_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
			      struct video_frmival_enum *fie);
int video_imager_set_fmt(const struct device *const dev, enum video_endpoint_id ep,
			 struct video_format *fmt);
int video_imager_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			 struct video_format *fmt);
int video_imager_get_caps(const struct device *dev, enum video_endpoint_id ep,
			  struct video_caps *caps);

/*
 * Initialize an imager by loading init_regs onto the device, and setting the default format.
 */
int video_imager_init(const struct device *dev, const struct video_cci_reg *init_regs,
		      int default_fmt_idx);

#endif /* ZEPHYR_DRIVERS_VIDEO_IMAGER_H_ */
