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
	struct video_device *vdev;
	struct video_caps caps = {.type = fmt->type};
	struct video_format *vdev_fmt;
	size_t idx;

	if (dev == NULL || fmt == NULL) {
		return -EINVAL;
	}

	switch (fmt->type) {
	case VIDEO_BUF_TYPE_INPUT:
		vdev_fmt = &vdev->in_fmt;
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		vdev_fmt = &vdev->out_fmt;
		break;
	default:
		LOG_ERR("Invlid buffer type %u requested", fmt->type);
		return -EINVAL;
	}

	vdev = dev->data;

	ret = video_get_caps(dev, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to query %s supported formats (type %u): %u",
			dev->name, fmt->type, ret);
		return ret;
	}

	ret = video_format_caps_index(fmt, &caps->format_caps, &idx);
	if (ret < 0) {
		LOG_ERR("Format %s %ux%u not supported by %s (type %u): %u",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
			dev->name, fmt->type, ret);
		return ret;
	}

	/* Capable of exchanging buffers: set a default format size which the driver may modify. */
	if (caps.min_vbuf_count > 0) {
		ret = video_estimate_fmt_size(fmt);
		if (ret < 0) {
			LOG_ERR("Failed to estimate the video %s %ux%u default size for %s",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height, dev->name);
			return ret;
		}
	}

	ret = video_driver_set_format(dev, fmt, idx);
	if (ret < 0) {
		LOG_ERR("Failed to apply format %s %ux%u to %s: %u",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height, dev->name,
			ret);
		return ret;
	}

	/* Store the format applied */
	*vdev_fmt = *fmt;

	return 0;
}

int video_get_format(const struct device *dev, struct video_format *fmt)
{
	struct video_device *vdev;

	if (dev == NULL || fmt == NULL) {
		return -EINVAL;
	}

	vdev = dev->data;

	switch (fmt->type) {
	case VIDEO_BUF_TYPE_INPUT:
		*fmt = vdev->in_fmt;
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		*fmt = vdev->out_fmt;
		break;
	default:
		LOG_ERR("Invlid buffer type %u requested", fmt->type);
		return -EINVAL;
	}

	return 0;
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
		fmt->pitch = fmt->width;
		fmt->size = fmt->pitch * fmt->height * 2U;
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
