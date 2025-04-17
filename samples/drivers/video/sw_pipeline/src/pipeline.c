/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/sw_pipeline.h>
#include <zephyr/logging/log.h>
#include <zephyr/pixel/formats.h>
#include <zephyr/pixel/resize.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sw_pipeline_definition, LOG_LEVEL_INF);

#define WIDTH_IN 64
#define HEIGHT_IN 64

#define WIDTH_OUT 128
#define HEIGHT_OUT 32

/* Color tuning converting one line of data */
void app_tune_rgb24line(const uint8_t *rgb24in, uint8_t *rgb24out, uint16_t width)
{
	for (size_t w = 0; w < width; w++) {
		rgb24out[w * 3 + 0] = CLAMP((-64 + rgb24in[w * 3 + 0]) * 2, 0x00, 0xff);
		rgb24out[w * 3 + 1] = CLAMP((-64 + rgb24in[w * 3 + 1]) * 1, 0x00, 0xff);
		rgb24out[w * 3 + 2] = CLAMP((-64 + rgb24in[w * 3 + 2]) * 2, 0x00, 0xff);
	}
}

/* Declare the pipeline elements and their intermediate buffers */
PIXEL_RGB565LESTREAM_TO_RGB24STREAM(step_rgb565_to_rgb24, WIDTH_IN, HEIGHT_IN);
PIXEL_SUBSAMPLE_RGB24STREAM(step_resize_rgb24, WIDTH_IN, HEIGHT_IN);
PIXEL_RGB24LINE_DEFINE(step_tune_rgb24, app_tune_rgb24line, WIDTH_OUT, HEIGHT_OUT);
PIXEL_RGB24STREAM_TO_YUYVSTREAM_BT709(step_rgb24_to_yuyv, WIDTH_OUT, HEIGHT_OUT);

int app_init_pipeline(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(video_sw_pipeline));
	struct pixel_stream *strm;

	/* Build the pipeline */
	strm = pixel_stream(&step_rgb565_to_rgb24, &step_resize_rgb24, &step_tune_rgb24,
			    &step_rgb24_to_yuyv, NULL);

	/* Load it into the driver */
	video_sw_pipeline_set_pipeline(dev, strm, VIDEO_PIX_FMT_RGB565, WIDTH_IN, HEIGHT_IN,
				       VIDEO_PIX_FMT_YUYV, WIDTH_OUT, HEIGHT_OUT);

	return 0;
}

/* Initialize before UVC so it can generate the USB descriptors from the formats above */
SYS_INIT(app_init_pipeline, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
