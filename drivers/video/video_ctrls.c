/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>

#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_ctrls, CONFIG_VIDEO_LOG_LEVEL);

static inline int check_range(enum video_ctrl_type type, int32_t min, int32_t max, uint32_t step,
			      int32_t def)
{
	switch (type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		if (step != 1 || max > 1 || min < 0) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_INTEGER:
	case VIDEO_CTRL_TYPE_INTEGER64:
		if (step == 0 || min > max || !IN_RANGE(def, min, max)) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_MENU:
		if (!IN_RANGE(min, 0, max) || !IN_RANGE(def, min, max)) {
			return -ERANGE;
		}
		return 0;
	default:
		return 0;
	}
}

static inline void set_type_flag(uint32_t id, enum video_ctrl_type *type, uint32_t *flags)
{
	*flags = 0;

	switch (id) {
	case VIDEO_CID_HFLIP:
	case VIDEO_CID_VFLIP:
		*type = VIDEO_CTRL_TYPE_BOOLEAN;
		break;
	case VIDEO_CID_POWER_LINE_FREQUENCY:
	case VIDEO_CID_TEST_PATTERN:
		*type = VIDEO_CTRL_TYPE_MENU;
		break;
	case VIDEO_CID_PIXEL_RATE:
		*type = VIDEO_CTRL_TYPE_INTEGER64;
		*flags |= VIDEO_CTRL_FLAG_READ_ONLY;
		break;
	default:
		*type = VIDEO_CTRL_TYPE_INTEGER;
		break;
	}
}

int video_init_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id, int32_t min,
		    int32_t max, uint32_t step, int32_t def)
{
	int ret;
	uint32_t flags;
	enum video_ctrl_type type;
	struct video_device *vdev = video_find_vdev(dev);

	if (!vdev) {
		return -EINVAL;
	}

	/* Sanity checks */
	if (id < VIDEO_CID_BASE) {
		return -EINVAL;
	}

	set_type_flag(id, &type, &flags);

	ret = check_range(type, min, max, step, def);
	if (ret) {
		return ret;
	}

	ctrl->vdev = vdev;
	ctrl->id = id;
	ctrl->type = type;
	ctrl->flags = flags;
	ctrl->min = min;
	ctrl->max = max;
	ctrl->step = step;
	ctrl->def = def;
	ctrl->val = def;

	sys_dlist_append(&vdev->ctrls, &ctrl->node);

	return 0;
}

static int video_find_ctrl(const struct device *dev, uint32_t id, struct video_ctrl **ctrl)
{
	struct video_device *vdev = video_find_vdev(dev);

	while (vdev) {
		SYS_DLIST_FOR_EACH_CONTAINER(&vdev->ctrls, *ctrl, node) {
			if ((*ctrl)->id == id) {
				return 0;
			}
		}

		vdev = video_find_vdev(vdev->src_dev);
	}

	return -ENOTSUP;
}

int video_get_ctrl(const struct device *dev, struct video_control *control)
{
	struct video_ctrl *ctrl = NULL;

	int ret = video_find_ctrl(dev, control->id, &ctrl);

	if (ret) {
		return ret;
	}

	if (ctrl->flags & VIDEO_CTRL_FLAG_WRITE_ONLY) {
		LOG_ERR("Control id 0x%x is write-only\n", control->id);
		return -EACCES;
	}

	control->val = ctrl->val;

	return 0;
}

int video_set_ctrl(const struct device *dev, struct video_control *control)
{
	struct video_ctrl *ctrl = NULL;

	int ret = video_find_ctrl(dev, control->id, &ctrl);

	if (ret) {
		return ret;
	}

	if (ctrl->flags & VIDEO_CTRL_FLAG_READ_ONLY) {
		LOG_ERR("Control id 0x%x is read-only\n", control->id);
		return -EACCES;
	}

	if (!IN_RANGE(control->val, ctrl->min, ctrl->max)) {
		LOG_ERR("Control value is invalid\n");
		return -EINVAL;
	}

	if (DEVICE_API_GET(video, ctrl->vdev->dev)->set_ctrl == NULL) {
		goto update;
	}

	/* Call driver's set_ctrl */
	ret = DEVICE_API_GET(video, ctrl->vdev->dev)->set_ctrl(ctrl->vdev->dev, control);
	if (ret) {
		return ret;
	}

update:
	/* Only update the ctrl in memory once everything is OK */
	ctrl->val = control->val;

	return 0;
}
