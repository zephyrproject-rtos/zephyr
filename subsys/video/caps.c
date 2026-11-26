/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

LOG_MODULE_REGISTER(video_caps, CONFIG_VIDEO_LOG_LEVEL);

#define VIDEO_CHAIN_MAX_DEPTH 8 /* Enough for most situations */

struct video_dev_index {
	/** The device at that depth that will be queried */
	const struct device *dev;
	/** Index through the capabilities entries for this level */
	size_t index;
};

int video_get_caps(const struct device *dev, struct video_caps *caps)
{
	if (dev == NULL || caps == NULL ||
	    (caps->type != VIDEO_BUF_TYPE_INPUT && caps->type != VIDEO_BUF_TYPE_OUTPUT)) {
		LOG_ERR("Invalid parameters (%p, %p)", (void *)dev, (void *)caps);
		return -EINVAL;
	}

	return video_driver_get_caps(dev, caps);
}

/* Apply a transformation from the source format all the way up through the chain of devices to get
 * the final format that the buffer is going to receive out of the chain of devices.
 */
static int video_transform_chain(struct video_dev_index *chain, size_t chain_len,
				 const struct video_format_cap *root_caps,
				 struct video_format_cap *dst_cap)
{
	struct video_format_cap src_cap = root_caps[chain[chain_len - 1].index];
	int ret;

	if (root_caps[chain[chain_len - 1].index].pixelformat == 0) {
		return -ERANGE;
	}

	for (int depth = (int)chain_len - 2; depth >= 0; depth--) {
		ret = video_transform_cap(chain[depth].dev, &src_cap, dst_cap,
					  VIDEO_BUF_TYPE_OUTPUT, chain[depth].index);
		if (ret == -ENOTSUP) {
			LOG_DBG("Format cap %s %ux%u ~ %ux%u (step %ux%u) not supported by %s",
				VIDEO_FOURCC_TO_STR(dst_cap->pixelformat),
				dst_cap->width_min, dst_cap->height_min,
				dst_cap->width_max, dst_cap->height_max,
				dst_cap->width_step, dst_cap->height_step,
				chain[depth].dev->name);

			/* Eliminate the full branch of unsupported format */
			chain[depth].index++;
			for (depth--; depth > 0; depth--) {
				chain[depth].index = 0;
			}

			return ret;
		}
		if (ret < 0) {
			return ret;
		}

		LOG_DBG("%s: %s %ux%u ~ %ux%u (step %ux%u) -> %s %ux%u ~ %ux%u (step %ux%u)",
			chain[depth].dev->name,

			VIDEO_FOURCC_TO_STR(src_cap.pixelformat),
			src_cap.width_min, src_cap.height_min,
			src_cap.width_max, src_cap.height_max,
			src_cap.width_step, src_cap.height_step,

			VIDEO_FOURCC_TO_STR(dst_cap->pixelformat),
			dst_cap->width_min, dst_cap->height_min,
			dst_cap->width_max, dst_cap->height_max,
			dst_cap->width_step, dst_cap->height_step);

		src_cap = *dst_cap;
	}

	return 0;
}

int video_get_format_caps(const struct device *dev, struct video_format_cap *dst_caps,
			  size_t num_dst_caps)
{
	struct video_dev_index chain[VIDEO_CHAIN_MAX_DEPTH] = {};
	size_t chain_len;
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	struct video_device *vdev;
	int ret;

	if (dev == NULL || dst_caps == NULL) {
		LOG_ERR("Invalid parameters (%p, %p)", (void *)dev);
		return -EINVAL;
	}

	vdev = dev->data;

	LOG_INF("Get format caps %s, %p, %u", dev->name, dst_caps, num_dst_caps);

	/* Fill the query chain of devices to speed-up next requests */
	for (chain_len = 0;; chain_len++) {
		if (chain_len == ARRAY_SIZE(chain)) {
			LOG_ERR("Maximum query depth %u reached", ARRAY_SIZE(chain));
			return -ENOBUFS;
		}

		chain[chain_len].dev = vdev->dev;

		LOG_DBG("getting %s capabilities", vdev->dev->name);

		ret = video_driver_get_caps(vdev->dev, &caps);
		if (ret < 0) {
			LOG_ERR("Failed to get %s format caps", vdev->dev->name);
			return ret;
		}

		if (caps.format_caps != NULL) {
			LOG_DBG("%s (%u) responded with format capabilities",
				vdev->dev->name, chain_len);
			break;
		}

		if (vdev->source_dev == NULL) {
			LOG_ERR("No video device in the chain provides a format list");
			return -ENOENT;
		}
		vdev = vdev->source_dev->data;
	}
	chain_len++;

	/* For the null terminator */
	memset(dst_caps, 0x00, sizeof(*dst_caps) * num_dst_caps);

	/* No transformation */
	if (chain_len == 1) {
		for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
			if (i + 1 >= num_dst_caps) {
				LOG_WRN("Not enough format caps (%u) to fill the full table",
					num_dst_caps);
				return -ENOBUFS;
			}
			dst_caps[i] = caps.format_caps[i];
		}
		return 0;
	}

	/* Travel the full recursive chain of devices and generate a flat table of formats */
	for (size_t chain_idx = 0, dst_idx = 0;;) {
		if (dst_idx + 1 >= num_dst_caps) {
			LOG_ERR("Out of buffer space to store the capabilities");
			return -ENOBUFS;
		}

		ret = video_transform_chain(chain, chain_len,
					    caps.format_caps, &dst_caps[dst_idx]);
		if (ret == 0) {
			chain_idx = 0;
			chain[chain_idx].index++;
			dst_idx++;

		} else if (ret == -ENOTSUP) {
			/* Ignore */

		} else if (ret == -ERANGE) {
			/* End of the iteration for this level, iterate the source device and
			 * try again
			 */
			chain[chain_idx].index = 0;

			chain_idx++;
			if (chain_idx >= chain_len) {
				/* No parent device */
				break;
			}

			chain[chain_idx].index++;

		} else {
			LOG_ERR("Failed to convert format across the chain of devices: %d", ret);
			return ret;
		}
	}

	return 0;
}

int video_format_caps_index(const struct video_format_cap *fmts, const struct video_format *fmt,
			    size_t *idx)
{
	if (fmts == NULL || fmt == NULL || idx == NULL) {
		LOG_ERR("Invalid arguments: (%p, %p, %p)", fmts, fmt, idx);
		return -EINVAL;
	}

	for (int i = 0; fmts[i].pixelformat != 0; i++) {
		LOG_INF("%s %ux%u ~ %ux%u (step %ux%u)",
			VIDEO_FOURCC_TO_STR(fmts[i].pixelformat),
			fmts[i].width_min, fmts[i].height_min,
			fmts[i].width_max, fmts[i].height_max,
			fmts[i].width_step, fmts[i].height_step);
	}

	for (int i = 0; fmts[i].pixelformat != 0; i++) {
		if (fmts[i].pixelformat == fmt->pixelformat &&
		    IN_RANGE(fmt->width, fmts[i].width_min, fmts[i].width_max) &&
		    IN_RANGE(fmt->height, fmts[i].height_min, fmts[i].height_max) &&
		    (fmts[i].width_step == 0 || fmt->width % fmts[i].width_step == 0) &&
		    (fmts[i].height_step == 0 || fmt->height % fmts[i].height_step == 0)) {
			*idx = i;
			return 0;
		}
	}
	return -ENOENT;
}

int video_transform_cap(const struct device *const dev,
			const struct video_format_cap *const cap,
			struct video_format_cap *const res_cap,
			enum video_buf_type type, uint16_t ind)
{
	int ret;

	if (dev == NULL || cap == NULL || res_cap == NULL ||
	    (type != VIDEO_BUF_TYPE_INPUT && type != VIDEO_BUF_TYPE_OUTPUT)) {
		LOG_ERR("Invalid parameters (%p, %p, %p, %d, %u)",
			(void *)dev, (void *)cap, (void *)res_cap, type, ind);
		return -EINVAL;
	}

	ret = video_driver_transform_cap(dev, cap, res_cap, type, ind);
	if (ret == -ENOSYS) {
		/* Default action: mirror the source capabilities unchanged */
		*res_cap = *cap;
	} else if (ret == -ENOTSUP || ret == -ERANGE) {
		return ret;
	} else if (ret < 0) {
		LOG_ERR("Failed to query %s's format capabilities: %d", dev->name, ret);
		return ret;
	}

	return 0;
}
