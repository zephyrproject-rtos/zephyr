/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_format, CONFIG_VIDEO_LOG_LEVEL);

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

static int video_check_format_caps(const struct device *dev, struct video_format *fmt)
{
	struct video_caps caps = {.type = fmt->type};
	size_t idx;
	int ret;

	ret = video_get_caps(dev, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to query %s supported formats (type %u): %d",
			dev->name, fmt->type, ret);
		return ret;
	}

	ret = video_format_caps_index(caps.format_caps, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Format %s %ux%u not supported by %s (type %u): %d",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
			dev->name, fmt->type, ret);
		return ret;
	}

	return 0;
}

int video_set_selection(const struct device *dev, struct video_selection *sel)
{
	struct video_device *vdev;
	struct video_format *dev_fmt;
	struct video_format fmt = {.type = sel->type};
	struct video_caps caps = {.type = sel->type};
	int ret;

	if (dev == NULL || sel == NULL || sel->type == 0) {
		LOG_ERR("Invalid parameters (%p, %p)", (void *)dev, (void *)sel);
		return -EINVAL;
	}

	vdev = dev->data;

	ret = video_get_caps(dev, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to query %s supported formats (type %u): %d",
			dev->name, fmt.type, ret);
		goto end;
	}

	k_mutex_lock(&vdev->lock, K_FOREVER);

	if (vdev->is_streaming & BIT(fmt.type)) {
		LOG_DBG("%s device is currently streaming", dev->name);
		ret = -EBUSY;
		goto end;
	}

	k_mutex_unlock(&vdev->lock);

	/* Find a device that responds to this API */
	while (true) {
		k_mutex_lock(&vdev->lock, K_FOREVER);

		/* Check if any of the video device of the chain does lacks format support */
		ret = video_check_format_caps(dev, &fmt);
		if (ret < 0) {
			goto end;
		}

		ret = video_driver_set_selection(vdev->dev, sel);
		if (ret < 0 && ret != -ENOSYS) {
			LOG_ERR("Failed to apply %s video selection to [%u, %u] %ux%u",
				dev->name, sel->rect.top, sel->rect.left, sel->rect.width,
				sel->rect.height);
			goto end;
		}
		if (ret == 0) {
			/* Lock still held */
			break;
		}

		k_mutex_unlock(&vdev->lock);

		if (vdev->source_dev == NULL) {
			return -ENOSYS;
		}
		vdev = vdev->source_dev->data;
	}

	switch (sel->type) {
	case VIDEO_BUF_TYPE_INPUT:
		dev_fmt = &vdev->in_fmt;
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		dev_fmt = &vdev->out_fmt;
		break;
	default:
		LOG_ERR("Invlid buffer type %u requested", fmt.type);
		ret = -EINVAL;
		goto end;
	}

	switch (sel->target) {
	case VIDEO_SEL_TGT_CROP:
		dev_fmt->width = sel->rect.width;
		dev_fmt->height = sel->rect.height;
		break;
	case VIDEO_SEL_TGT_COMPOSE:
		dev_fmt->width = sel->rect.width + sel->rect.left;
		dev_fmt->height = sel->rect.height + sel->rect.top;
		break;
	default:
		ret = -EINVAL;
		goto end;
	}

end:
	k_mutex_unlock(&vdev->lock);

	return ret;
}

int video_get_selection(const struct device *dev, struct video_selection *sel)
{
	if (dev == NULL || sel == NULL || sel->type == 0) {
		LOG_ERR("Invalid parameters (%p, %p)", (void *)dev, (void *)sel);
		return -EINVAL;
	}

	return video_driver_get_selection(dev, sel);
}

static int video_apply_format(const struct device *dev, struct video_format *fmt)
{
	struct video_caps caps = {.type = fmt->type};
	struct video_device *vdev = dev->data;
	struct video_format *vdev_fmt;
	struct video_format prev_fmt;
	size_t idx;
	int ret;

	if (vdev->is_streaming & BIT(fmt->type)) {
		return -EBUSY;
	}

	switch (fmt->type) {
	case VIDEO_BUF_TYPE_INPUT:
		vdev_fmt = &vdev->in_fmt;
		vdev->in_compose.top = 0;
		vdev->in_compose.left = 0;
		vdev->in_compose.width = fmt->width;
		vdev->in_compose.height = fmt->height;
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		vdev_fmt = &vdev->out_fmt;
		break;
	default:
		LOG_ERR("Invalid format type %d", fmt->type);
		return -EINVAL;
	}

	if (memcmp(vdev_fmt, fmt, sizeof(*vdev_fmt)) == 0) {
		LOG_DBG("Format %s %ux%u already applied to %s",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height, dev->name);
		return 0;
	}

	ret = video_get_caps(dev, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to query %s supported formats (type %u): %d",
			dev->name, fmt->type, ret);
		return ret;
	}

	ret = video_format_caps_index(caps.format_caps, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Format %s %ux%u not supported by %s (type %u): %d",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
			dev->name, fmt->type, ret);
		return ret;
	}

	/* Capable of exchanging buffers: set a default format size which the driver may modify. */
	if (caps.min_vbuf_count > 0) {
		ret = video_estimate_fmt_size(fmt);
		if (ret < 0) {
			LOG_ERR("Failed to estimate the video %s %ux%u default size for %s",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
				dev->name);
			return ret;
		}
	}

	prev_fmt = *vdev_fmt;
	*vdev_fmt = *fmt;

	ret = video_driver_set_format(dev, fmt, idx);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Failed to set %s format to %s %ux%u: %d",
			dev->name, VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
			ret);
		*vdev_fmt = prev_fmt;
		return ret;
	}

	/* May be -ENOSYS */
	return ret;
}

int video_set_format(const struct device *dev, struct video_format *fmt)
{
	struct video_device *vdev;
	int ret;

	if (dev == NULL || fmt == NULL) {
		return -EINVAL;
	}

	vdev = dev->data;

	/* Fallback implementation: set the format of the source device */
	while (true) {
		struct video_format_cap src_fcap = {
			.width_min = fmt->width, .height_min = fmt->height,
			.width_max = fmt->width, .height_max = fmt->height,
			.pixelformat = fmt->pixelformat,
		};
		struct video_format_cap dst_fcap = {};

		k_mutex_lock(&vdev->lock, K_FOREVER);

		ret = video_apply_format(vdev->dev, fmt);
		if (ret == 0) {
			break;
		}

		k_mutex_unlock(&vdev->lock);

		if (ret < 0 && ret != -ENOSYS) {
			LOG_ERR("Failed to apply format %s %ux%u to %s: %u",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
				vdev->dev->name, ret);
			return ret;
		}

		/* Pick a source format to reflect the changes done by the driver,
		 * so that it is possible to configure the source device.
		 *
		 * TODO: this is an heuristic that picks the first (0) result, which might not work
		 * further down the chain.
		 *
		 * A full solution would be to have per-platform strategies to figure out the most
		 * suited format for every device.
		 */
		ret = video_transform_cap(vdev->dev, &src_fcap, &dst_fcap,
					  VIDEO_BUF_TYPE_OUTPUT, 0);
		if (ret < 0) {
			return ret;
		}

		fmt->width = dst_fcap.width_max;
		fmt->height = dst_fcap.height_max;
		fmt->pixelformat = dst_fcap.pixelformat;

		if (vdev->source_dev == NULL) {
			LOG_ERR("No device support setting the format");
			return -ENOSYS;
		}
		vdev = vdev->source_dev->data;
	}

	k_mutex_unlock(&vdev->lock);

	return ret;
}

int video_get_format(const struct device *dev, struct video_format *fmt)
{
	struct video_device *vdev;

	if (dev == NULL || fmt == NULL || fmt->type == 0) {
		LOG_ERR("Invalid parameters (%p, %p)", (void *)dev, (void *)fmt);
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
	if (fmt == NULL || fmt->type == 0) {
		LOG_ERR("Invalid parameters (%p)", (void *)fmt);
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

static int video_closest_format_by_size(const struct device *dev,
					const struct video_format *fmt,
					struct video_format *match)
{
	struct video_caps caps = {.type = fmt->type};
	int ret;

	ret = video_get_caps(dev, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to query %s formats to apply scaling", dev->name);
		return ret;
	}

	match->width = UINT16_MAX;
	match->height = UINT16_MAX;

	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		int32_t width;
		int32_t height;

		if (fmt->pixelformat != caps.format_caps[i].pixelformat) {
			continue;
		}

		width = (fmt->width - caps.format_caps[i].width_min)
			/ caps.format_caps[i].width_step
			* caps.format_caps[i].width_step
			+ caps.format_caps[i].width_min;

		height = (fmt->height - caps.format_caps[i].height_min)
			/ caps.format_caps[i].height_step
			* caps.format_caps[i].height_step
			+ caps.format_caps[i].height_min;

		if (ABS((int32_t)fmt->width - width) *
		    ABS((int32_t)fmt->height - height) <
		    ABS((int32_t)fmt->width - match->width) *
		    ABS((int32_t)fmt->height - match->height)) {
			match->width = width;
			match->height = height;
		}
	}

	if (match->width == UINT16_MAX || match->height == UINT16_MAX) {
		return -ENOENT;
	}

	ret = video_estimate_fmt_size(match);
	if (ret < 0) {
		return ret;
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
	struct video_format match = {.type = fmt->type, .pixelformat = fmt->pixelformat};
	int ret;

	ret = video_set_format(dev, fmt);
	if (ret == 0) {
		/* Format supported without scaling, no need to try further  */
		return 0;
	}

	LOG_DBG("Failed to apply '%s' %ux%u to %s directly, falling back to scaling",
		VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height, dev->name);

	/* TODO: should be platform-specific choices/policies */
	ret = video_closest_format_by_size(dev, fmt, &match);
	if (ret < 0) {
		LOG_ERR("Failed to get %s closest matching format to '%s' %ux%u selected: %d",
			dev->name, VIDEO_FOURCC_TO_STR(match.pixelformat),
			match.width, match.height, ret);
		return ret;
	}

	ret = video_set_format(dev, &match);
	if (ret < 0) {
		LOG_ERR("Unable to set %s to '%s' %ux%u before scaling: %d",
			dev->name, VIDEO_FOURCC_TO_STR(match.pixelformat),
			match.width, match.height, ret);
		return ret;
	}

	ret = video_set_selection(dev, &sel);
	if (ret < 0) {
		LOG_ERR("Unable to configure %s scaling to %ux%u",
			dev->name, sel.rect.width, sel.rect.height);
		return ret;
	}

	return 0;
}
