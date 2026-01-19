/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(transform, CONFIG_LOG_DEFAULT_LEVEL);

static inline int setup_video_transform(const struct device *const transform_dev,
					struct video_format in_fmt,
					struct video_format *const out_fmt,
					struct video_buffer **out_buf)
{
	int ret;

	/* Set input and output formats */
	in_fmt.type = VIDEO_BUF_TYPE_INPUT;
	ret = video_set_format(transform_dev, &in_fmt);
	if (ret < 0) {
		LOG_ERR("Unable to set input format");
		return ret;
	}

	ret = video_set_format(transform_dev, out_fmt);
	if (ret < 0) {
		LOG_ERR("Unable to set output format");
		return ret;
	}

	/* Allocate a buffer for output queue */
	*out_buf = video_buffer_aligned_alloc(out_fmt->size,
					      CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_NO_WAIT);
	if (*out_buf == NULL) {
		LOG_ERR("Unable to allocate output video buffer");
		return -ENOMEM;
	}

	return 0;
}

static inline int transform_frame(const struct device *const transform_dev,
				  struct video_buffer *in_buf, struct video_buffer **out_buf)
{
	int err;
	/* Store the input buffer type to recover later */
	enum video_buf_type in_buf_type = in_buf->type;
	static bool started = false;

	in_buf->type = VIDEO_BUF_TYPE_INPUT;
	err = video_enqueue(transform_dev, in_buf);
	if (err < 0) {
		LOG_ERR("Unable to enqueue input buffer");
		return err;
	}

	(*out_buf)->type = VIDEO_BUF_TYPE_OUTPUT;
	err = video_enqueue(transform_dev, *out_buf);
	if (err < 0) {
		LOG_ERR("Unable to enqueue output buffer");
		return err;
	}

	if (!started) {
		err = video_stream_start(transform_dev, VIDEO_BUF_TYPE_INPUT);
		if (err < 0) {
			LOG_ERR("Unable to start the transform input");
			return err;
		}

		err = video_stream_start(transform_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (err < 0) {
			LOG_ERR("Unable to start the transform input");
			return err;
		}

		started = true;
	}

	err = video_dequeue(transform_dev, &in_buf, K_FOREVER);
	if (err < 0) {
		LOG_ERR("Unable to dequeue input buffer");
		return err;
	}

	err = video_dequeue(transform_dev, out_buf, K_FOREVER);
	if (err < 0) {
		LOG_ERR("Unable to dequeue output buffer");
		return err;
	}

	/* Keep the input buffer intact */
	in_buf->type = in_buf_type;

	return 0;
}
