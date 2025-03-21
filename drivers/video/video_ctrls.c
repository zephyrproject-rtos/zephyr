/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

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
	struct video_ctrl *vc;
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

	/* Insert in an ascending order of ctrl's id */
	SYS_DLIST_FOR_EACH_CONTAINER(&vdev->ctrls, vc, node) {
		if (vc->id > ctrl->id) {
			sys_dlist_insert(&vc->node, &ctrl->node);
			return 0;
		}
	}

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

static inline const char *video_get_ctrl_name(uint32_t id)
{
	switch (id) {
	/* User controls */
	case VIDEO_CID_BRIGHTNESS:
		return "Brightness";
	case VIDEO_CID_CONTRAST:
		return "Contrast";
	case VIDEO_CID_SATURATION:
		return "Saturation";
	case VIDEO_CID_HUE:
		return "Hue";
	case VIDEO_CID_EXPOSURE:
		return "Exposure";
	case VIDEO_CID_GAIN:
		return "Gain";
	case VIDEO_CID_HFLIP:
		return "Horizontal Flip";
	case VIDEO_CID_VFLIP:
		return "Vertical Flip";
	case VIDEO_CID_POWER_LINE_FREQUENCY:
		return "Power Line Frequency";

	/* Camera controls */
	case VIDEO_CID_ZOOM_ABSOLUTE:
		return "Zoom, Absolute";

	/* JPEG encoder controls */
	case VIDEO_CID_JPEG_COMPRESSION_QUALITY:
		return "Compression Quality";

	/* Image processing controls */
	case VIDEO_CID_PIXEL_RATE:
		return "Pixel Rate";
	case VIDEO_CID_TEST_PATTERN:
		return "Test Pattern";
	default:
		return NULL;
	}
}

int video_query_ctrl(const struct device *dev, struct video_ctrl_query *cq)
{
	int ret;
	struct video_device *vdev;
	struct video_ctrl *ctrl = NULL;

	if (cq->id & VIDEO_CTRL_FLAG_NEXT_CTRL) {
		vdev = video_find_vdev(dev);
		cq->id &= ~VIDEO_CTRL_FLAG_NEXT_CTRL;
		while (vdev) {
			SYS_DLIST_FOR_EACH_CONTAINER(&vdev->ctrls, ctrl, node) {
				if (ctrl->id > cq->id) {
					goto fill_query;
				}
			}
			vdev = video_find_vdev(vdev->src_dev);
		}
		return -ENOTSUP;
	}

	ret = video_find_ctrl(dev, cq->id, &ctrl);
	if (ret) {
		return ret;
	}

fill_query:
	cq->id = ctrl->id;
	cq->type = ctrl->type;
	cq->flags = ctrl->flags;
	cq->min = ctrl->min;
	cq->max = ctrl->max;
	cq->step = ctrl->step;
	cq->def = ctrl->def;
	cq->name = video_get_ctrl_name(cq->id);

	return 0;
}

void video_print_ctrl(const struct device *const dev, const struct video_ctrl_query *const cq)
{
	uint8_t i = 0;
	const char *type = NULL;
	char typebuf[8];

	__ASSERT(dev && cq, "Invalid arguments");

	/* Get type of the control */
	switch (cq->type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		type = "bool";
		break;
	case VIDEO_CTRL_TYPE_INTEGER:
		type = "int";
		break;
	case VIDEO_CTRL_TYPE_INTEGER64:
		type = "int64";
		break;
	case VIDEO_CTRL_TYPE_MENU:
		type = "menu";
		break;
	case VIDEO_CTRL_TYPE_STRING:
		type = "string";
		break;
	default:
		break;
	}
	snprintf(typebuf, sizeof(typebuf), "(%s)", type);

	/* Get current value of the control */
	struct video_control vc = {.id = cq->id};

	video_get_ctrl(dev, &vc);

	/* Print the control information */
	if (cq->type == VIDEO_CTRL_TYPE_INTEGER64) {
		LOG_INF("%32s 0x%08x %-8s (flags=0x%02x) : min=%lld max=%lld step=%lld "
			"default=%lld value=%lld ",
			cq->name, cq->id, typebuf, cq->flags, cq->range.min64, cq->range.max64,
			cq->range.step64, cq->range.def64, vc.val64);
	} else {
		LOG_INF("%32s 0x%08x %-8s (flags=0x%02x) : min=%d max=%d step=%d default=%d "
			"value=%d ",
			cq->name, cq->id, typebuf, cq->flags, cq->range.min, cq->range.max,
			cq->range.step, cq->range.def, vc.val);
	}
}
