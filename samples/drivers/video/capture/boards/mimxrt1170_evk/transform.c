/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(transform, CONFIG_LOG_DEFAULT_LEVEL);

static const struct device *transform_dev;
static struct video_buffer *transformed_buffer;

int video_transform_setup(struct video_format in_fmt, struct video_format *out_fmt)
{
	int ret;
	struct video_control ctrl = {.id = VIDEO_CID_ROTATE, .val = 90};

	transform_dev = DEVICE_DT_GET(DT_NODELABEL(pxp));

	out_fmt->pixelformat = in_fmt.pixelformat;
	out_fmt->width = in_fmt.height;
	out_fmt->height = in_fmt.width;
	out_fmt->type = VIDEO_BUF_TYPE_OUTPUT;

	/* Set input and output formats */
	in_fmt.type = VIDEO_BUF_TYPE_INPUT;
	ret = video_set_format(transform_dev, &in_fmt);
	if (ret) {
		LOG_ERR("Unable to set input format");
		return ret;
	}

	ret = video_set_format(transform_dev, out_fmt);
	if (ret) {
		LOG_ERR("Unable to set output format");
		return ret;
	}

	/* Set controls */
	video_set_ctrl(transform_dev, &ctrl);

	/* Allocate a buffer for output queue */
	transformed_buffer = video_buffer_aligned_alloc(in_fmt.pitch * in_fmt.height,
						CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_FOREVER);

	if (transformed_buffer == NULL) {
		LOG_ERR("Unable to alloc video buffer");
		return -ENOMEM;
	}

	return 0;
}

int video_transform_process(struct video_buffer *in_buf, struct video_buffer **out_buf)
{
	int err;
	struct video_buffer *transformed_buf = &(struct video_buffer){};
	/* Store the input buffer type to recover later */
	enum video_buf_type in_buf_type = in_buf->type;

	in_buf->type = VIDEO_BUF_TYPE_INPUT;
	err = video_enqueue(transform_dev, in_buf);
	if (err) {
		LOG_ERR("Unable to enqueue input buffer");
		return err;
	}

	transformed_buffer->type = VIDEO_BUF_TYPE_OUTPUT;
	err = video_enqueue(transform_dev, transformed_buffer);
	if (err) {
		LOG_ERR("Unable to enqueue output buffer");
		return err;
	}

	/* Same start for both input and output */
	if (video_stream_start(transform_dev, VIDEO_BUF_TYPE_INPUT)) {
		LOG_ERR("Unable to start PxP");
		return 0;
	}

	/* Dequeue input and output buffers */
	err = video_dequeue(transform_dev, &in_buf, K_FOREVER);
	if (err) {
		LOG_ERR("Unable to dequeue PxP video buf in");
		return err;
	}

	transformed_buf->type = VIDEO_BUF_TYPE_OUTPUT;
	err = video_dequeue(transform_dev, &transformed_buf, K_FOREVER);
	if (err) {
		LOG_ERR("Unable to dequeue PxP video buf out");
		return err;
	}

	*out_buf = transformed_buf;

	/* Keep the input buffer intact */
	in_buf->type = in_buf_type;

	return 0;
}
