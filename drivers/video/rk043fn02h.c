/*
 * Copyright (c) 2019, Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rocktech_rk043fn02h_ct

#include <zephyr.h>
#include <device.h>

#include <drivers/video.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(rk043fn02h);

/* Static Resolution 480*272 RGB565 */
#define RK043FN02H_WIDTH 480
#define RK043FN02H_HEIGHT 272

static int rk043fn02h_set_fmt(struct device *dev, enum video_endpoint_id ep,
			      struct video_format *fmt)
{
	if (ep != VIDEO_EP_IN)
		return -EINVAL;

	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565) {
		return -ENOTSUP;
	}

	if (fmt->width < RK043FN02H_WIDTH || fmt->height < RK043FN02H_HEIGHT) {
		return -ENOTSUP;
	} else if (fmt->width > RK043FN02H_WIDTH ||
		   fmt->height > RK043FN02H_HEIGHT) {
		/* we accept bigger resolution than display resolution.
		 * In that case, image is cropped.
		 */
		 LOG_WRN("Image will be cropped");
	}

	return 0;
}

static int rk043fn02h_get_fmt(struct device *dev, enum video_endpoint_id ep,
			      struct video_format *fmt)
{
	if (ep != VIDEO_EP_IN)
		return -EINVAL;

	fmt->pixelformat = VIDEO_PIX_FMT_RGB565;
	fmt->width = RK043FN02H_WIDTH;
	fmt->height = RK043FN02H_HEIGHT;
	fmt->pitch = RK043FN02H_WIDTH * 2;

	return 0;
}

static int rk043fn02h_stream_start(struct device *dev)
{
	/* Nothing to do (TODO: gpio mgmt)*/
	return 0;
}

static int rk043fn02h_stream_stop(struct device *dev)
{
	/* Nothing to do (TODO: gpio mgmt) */
	return 0;
}

static const struct video_format_cap fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width_min = RK043FN02H_WIDTH,
		.width_max = RK043FN02H_WIDTH * 2,
		.height_min = RK043FN02H_HEIGHT,
		.height_max = RK043FN02H_HEIGHT * 2,
		.width_step = 1,
		.height_step = 1,
	},
	{ 0 }
};

static int rk043fn02h_get_caps(struct device *dev, enum video_endpoint_id ep,
			       struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static const struct video_driver_api rk043fn02h_driver_api = {
	.set_format = rk043fn02h_set_fmt,
	.get_format = rk043fn02h_get_fmt,
	.get_caps = rk043fn02h_get_caps,
	.stream_start = rk043fn02h_stream_start,
	.stream_stop = rk043fn02h_stream_stop,
};

static int rk043fn02h_init(struct device *dev)
{
	/* Nothing to do */
	return 0;
}

DEVICE_AND_API_INIT(rk043fn02h, DT_INST_LABEL(0),
		    &rk043fn02h_init, NULL, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &rk043fn02h_driver_api);
