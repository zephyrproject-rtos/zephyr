/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_format, CONFIG_VIDEO_LOG_LEVEL);

int video_set_selection(const struct device *dev, struct video_selection *sel)
{
	if (dev == NULL || sel == NULL) {
		return -EINVAL;
	}

	return video_driver_set_selection(dev, sel);
}

int video_get_selection(const struct device *dev, struct video_selection *sel)
{
	if (dev == NULL || sel == NULL) {
		return -EINVAL;
	}

	return video_driver_get_selection(dev, sel);
}

int video_set_format(const struct device *dev, struct video_format *fmt)
{
	if (dev == NULL || fmt == NULL) {
		return -EINVAL;
	}

	return video_driver_set_format(dev, fmt);
}

int video_get_format(const struct device *dev, struct video_format *fmt)
{
	if (dev == NULL || fmt == NULL) {
		return -EINVAL;
	}

	return video_driver_get_format(dev, fmt);
}

int video_estimate_fmt_size(struct video_format *fmt)
{
	if (fmt == NULL) {
		return -EINVAL;
	}

	switch (fmt->pixelformat) {
	case VIDEO_PIX_FMT_JPEG:
	case VIDEO_PIX_FMT_PNG:
	case VIDEO_PIX_FMT_H264:
		/* Rough estimate for the worst case (quality = 100) */
		fmt->pitch = 0;
		fmt->size = fmt->width * fmt->height * 2;
		break;
	case VIDEO_PIX_FMT_NV12:
	case VIDEO_PIX_FMT_NV21:
	case VIDEO_PIX_FMT_NV16:
	case VIDEO_PIX_FMT_NV61:
		fmt->pitch = fmt->width;
		fmt->size = fmt->pitch * fmt->height * 2U;
		break;
	case VIDEO_PIX_FMT_NV24:
	case VIDEO_PIX_FMT_NV42:
	case VIDEO_PIX_FMT_RGB24PLANAR:
	case VIDEO_PIX_FMT_YUV420:
	case VIDEO_PIX_FMT_YVU420:
		fmt->pitch = fmt->width;
		fmt->size = fmt->pitch * fmt->height * 3U;
		break;
	default:
		/* Uncompressed format */
		fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
		if (fmt->pitch == 0) {
			return -ENOTSUP;
		}
		fmt->size = fmt->pitch * fmt->height;
		break;
	}

	return 0;
}

uint32_t video_get_buffer_height(const struct video_buffer *vbuf, const struct video_format *fmt)
{
	unsigned int size_factor;

	if (vbuf == NULL || fmt == NULL || fmt->pitch == 0U) {
		LOG_ERR("Video buffer and format must be valid with a nonzero pitch");
		return 0U;
	}

	size_factor = video_num_planes(fmt->pixelformat);
	if (fmt->pixelformat == VIDEO_PIX_FMT_NV24 || fmt->pixelformat == VIDEO_PIX_FMT_NV42) {
		/* The interleaved chroma plane has twice the luma pitch. */
		size_factor = 3U;
	}

	return vbuf->bytesused / fmt->pitch / size_factor;
}

int video_set_compose_format(const struct device *dev, struct video_format *fmt)
{
	struct video_selection sel = {
		.type = fmt->type,
		.target = VIDEO_SEL_TGT_COMPOSE,
		.rect.left = 0,
		.rect.top = 0,
		.rect.width = fmt->width,
		.rect.height = fmt->height,
	};
	int ret;

	ret = video_set_selection(dev, &sel);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Unable to set selection compose");
		return ret;
	}

	return video_set_format(dev, fmt);
}
