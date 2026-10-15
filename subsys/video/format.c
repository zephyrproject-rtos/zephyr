/*
 * SPDX-FileCopyrightText: The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_format, CONFIG_VIDEO_LOG_LEVEL);

static int video_check_format_caps(const struct device *dev, struct video_format *fmt)
{
	size_t idx;
	int ret;

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

	return 0;
}

/* Reset the format applied to a whole chain of devices between first and last, both included */
static int video_propagate_format(const struct device *first, const struct device *last,
				  struct video_format *fmt)
{
	struct video_device *vdev = device->data;
	struct video_rect rect = {.left = 0, .top = 0, .width = fmt->width, .height = fmt->height};
	int ret = 0;

	do {
		k_mutex_lock(&vdev->lock, K_FOREVER);

		ret = video_check_format_caps(dev, fmt);
		if (ret < 0) {
			goto end;
		}

		/* Reset the format */
		if (fmt->type == VIDEO_BUF_TYPE_INPUT) {
			vdev->in_fmt = *fmt;
			vdev->in_selection = rect;
			vdev->in_composition = rect;
		} else {
			vdev->out_fmt = *fmt;
			vdev->out_selection = rect;
			vdev->out_composition = rect;
		}

		k_mutex_unlock(&vdev->lock);

		if (vdev->source_dev == NULL) {
			break;
		}

		vdev = vdev->source_dev->data;
	} while (vdev->dev != last);
end:
	return ret;
}

int video_set_selection(const struct device *dev, struct video_selection *sel)
{
	struct video_device *vdev;
	struct video_format *dev_fmt;
	struct video_format fmt = {.type = sel->type};
	struct video_caps caps = {.type = sel->type};
	size_t idx;
	int ret;

	if (dev == NULL || sel == NULL) {
		return -EINVAL;
	}

	ret = video_get_caps(dev, &caps);
	if (ret < 0) {
		LOG_ERR("Failed to query %s supported formats (type %u): %u",
			dev->name, fmt->type, ret);
		goto end;
	}

	vdev = dev->data;

	k_mutex_lock(&vdev->lock, K_FOREVER);

	if (vdev->is_streaming & BIT(fmt->type)) {
		LOG_DBG("%s device is currently streaming");
		ret = -EBUSY;
		goto end;
	}

	k_mutex_unlock(&vdev->lock);

	/* Find a device that responds to this API */
	while (true) {
		k_mutex_lock(&vdev->lock, K_FOREVER);

		/* Check if any of the video device of the chain does lacks format support */
		ret = video_check_format_caps(dev, fmt);
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
		vdev = vdev->source_dev;
	}

	switch (sel->type) {
	case VIDEO_BUF_TYPE_INPUT:
		dev_fmt = &vdev->in_fmt;
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		dev_fmt = &vdev->out_fmt;
		break;
	default:
		LOG_ERR("Invlid buffer type %u requested", fmt->type);
		ret = -EINVAL;
		goto end;
	}

	switch (sel_target) {
	case VIDEO_SEL_TGT_CROP:
		dev_fmt->width = sel->rect.width;
		dev_fmt->height = sel->rect.height;
		break;
	case VIDEO_SEL_TGT_CMOPOSE:
		dev_fmt->width = sel->rect.width + sel->rect.left;
		dev_fmt->height = sel->rect.height + sel->rect.top;
		break;
	default:
		ret = -EINVAL;
		goto end;
	}

	ret = video_propagate_format(dev, vdev->dev, dev_fmt);

	/* TODO: in addition to propagate the format configuration, also request the device to
	 * update its configuration. Two options:
	 *
	 * - Change apply_format semantics so that it does not propagate the format down the
	 *   chain again, as this would cancel the crop of all underlying devices if any
	 *
	 * - Change the semantics of set_format() to not apply the format for the underlying
	 *   devices, in which case there need to be an API to tell how the format is transformed
	 *   between the source and destination, such as reusing video_transform_cap for
	 *   subdevs-style devices. This would be used by the subsystem to reconfigure the
	 *   source devices whenever a format is applied at a higher level.
	 */
end:
	k_mutex_unlock(&vdev->lock);

	return ret;
}

int video_get_selection(const struct device *dev, struct video_selection *sel)
{
	struct video_device *vdev;
	int ret;

	if (dev == NULL || sel == NULL) {
		return -EINVAL;
	}

	vdev = dev->data;

	k_mutex_lock(&vdev->lock, K_FOREVER);

	ret = video_driver_get_selection(dev, sel);
end:
	k_mutex_unlock(&vdev->lock);

	return ret;
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

	vdev = dev->data;

	k_mutex_lock(&vdev->lock, K_FOREVER);

	switch (fmt->type) {
	case VIDEO_BUF_TYPE_INPUT:
		vdev_fmt = &vdev->in_fmt;
		vdev->compose.rect = sel->rect;
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		vdev_fmt = &vdev->out_fmt;
		break;
	default:
		LOG_ERR("Invlid buffer type %u requested", fmt->type);
		return -EINVAL;
	}

	if (memcmp(vdev_fmt, fmt, sizeof(*vdev_fmt)) == 0) {
		LOG_DBG("Format %s %ux%u already applied to %s",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height, dev->name);
		return 0;
	}

	if (vdev->is_streaming & BIT(fmt->type)) {
		LOG_DBG("%s device is currently streaming");
		ret = -EBUSY;
		goto end;
	}

	/* Able of exchanging buffers: set a default format size which the driver may modify. */
	if (caps.min_vbuf_count > 0) {
		ret = video_estimate_fmt_size(fmt);
		if (ret < 0) {
			LOG_ERR("Failed to estimate the video %s %ux%u default size for %s",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
				dev->name);
			goto end;
		}
	}

	/* Find a device that responds to this API */
	while (true) {
		k_mutex_lock(&vdev->lock, K_FOREVER);

		/* Check if any of the video device of the chain does not support this format */
		ret = video_check_format_caps(dev, fmt);
		if (ret < 0) {
			goto end;
		}

		ret = video_driver_set_format(dev, fmt, idx);
		if (ret < 0 && ret != -ENOSYS) {
			LOG_ERR("Failed to apply format %s %ux%u to %s: %u",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height,
				dev->name, ret);
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

	ret = video_propagate_format(dev, vdev->dev, fmt);
end:
	k_mutex_unlock(&vdev->lock);

	return ret;
}

int video_get_format(const struct device *dev, struct video_format *fmt)
{
	struct video_device *vdev;

	if (dev == NULL || fmt == NULL) {
		return -EINVAL;
	}

	vdev = dev->data;

	k_mutex_lock(&vdev->lock, K_FOREVER);

	switch (fmt->type) {
	case VIDEO_BUF_TYPE_INPUT:
		*fmt = vdev->in_fmt;
		break;
	case VIDEO_BUF_TYPE_OUTPUT:
		*fmt = vdev->out_fmt;
		break;
	default:
		LOG_ERR("Invlid buffer type %u requested", fmt->type);
		ret =  -EINVAL;
		goto end;
	}

end:
	k_mutex_unlock(&vdev->lock);

	return ret;
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
	struct video_device *vdev;
	struct video_selection sel = {
		.type = fmt->type,
		.target = VIDEO_SEL_TGT_COMPOSE,
		.rect.left = 0,
		.rect.top = 0,
		.rect.width = fmt->width,
		.rect.height = fmt->height,
	};
	int ret;

	vdev = dev->data;

	k_mutex_lock(&vdev->lock, K_FOREVER);

	ret = video_set_selection(dev, &sel);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Unable to set selection compose");
		return ret;
	}

	ret = video_set_format(dev, fmt);
end:
	k_mutex_unlock(&vdev->lock);

	return ret;
}
